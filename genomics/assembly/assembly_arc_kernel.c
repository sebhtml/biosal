
#include "assembly_arc_kernel.h"

#include "assembly_graph_store.h"
#include "assembly_arc_block.h"

#include <genomics/kernels/dna_kmer_counter_kernel.h>

#include <genomics/input/input_command.h>

#include <genomics/storage/sequence_store.h>

#include <stdio.h>

#include <stdint.h>
#include <inttypes.h>

struct thorium_script bsal_assembly_arc_kernel_script = {
    .identifier = SCRIPT_ASSEMBLY_ARC_KERNEL,
    .name = "bsal_assembly_arc_kernel",
    .init = bsal_assembly_arc_kernel_init,
    .destroy = bsal_assembly_arc_kernel_destroy,
    .receive = bsal_assembly_arc_kernel_receive,
    .size = sizeof(struct bsal_assembly_arc_kernel),
    .author = "Sebastien Boisvert",
    .description = "Isolate assembly arcs from entries in an input block",
    .version = "AlphaOmegaCool"
};

void bsal_assembly_arc_kernel_init(struct thorium_actor *self)
{
    struct bsal_assembly_arc_kernel *concrete_self;

    concrete_self = (struct bsal_assembly_arc_kernel *)thorium_actor_concrete_actor(self);

    concrete_self->kmer_length = -1;

    bsal_fast_queue_init(&concrete_self->producers_for_work_stealing, sizeof(int));

    thorium_actor_add_action(self, ACTION_SET_KMER_LENGTH,
                    bsal_assembly_arc_kernel_set_kmer_length);

    printf("%s/%d is now active\n",
                    thorium_actor_script_name(self),
                    thorium_actor_name(self));

    concrete_self->producer = THORIUM_ACTOR_NOBODY;
    concrete_self->consumer = THORIUM_ACTOR_NOBODY;

    /*
     * Configure the codec.
     */

    bsal_dna_codec_init(&concrete_self->codec);

    if (bsal_dna_codec_must_use_two_bit_encoding(&concrete_self->codec,
                            thorium_actor_get_node_count(self))) {
        bsal_dna_codec_enable_two_bit_encoding(&concrete_self->codec);
    }

    concrete_self->produced_arcs = 0;

    thorium_actor_add_action(self, ACTION_PUSH_SEQUENCE_DATA_BLOCK,
                    bsal_assembly_arc_kernel_push_sequence_data_block);
    thorium_actor_add_action(self, ACTION_SET_PRODUCERS_FOR_WORK_STEALING,
                    bsal_assembly_arc_kernel_set_producers_for_work_stealing);

    concrete_self->received_blocks = 0;

    concrete_self->flushed_messages = 0;
}

void bsal_assembly_arc_kernel_destroy(struct thorium_actor *self)
{
    struct bsal_assembly_arc_kernel *concrete_self;

    concrete_self = (struct bsal_assembly_arc_kernel *)thorium_actor_concrete_actor(self);

    concrete_self->kmer_length = -1;

    bsal_dna_codec_destroy(&concrete_self->codec);

    concrete_self->producer = THORIUM_ACTOR_NOBODY;
    concrete_self->consumer = THORIUM_ACTOR_NOBODY;

    bsal_fast_queue_destroy(&concrete_self->producers_for_work_stealing);
}

void bsal_assembly_arc_kernel_receive(struct thorium_actor *self, struct thorium_message *message)
{
    int tag;
    int source;
    struct bsal_assembly_arc_kernel *concrete_self;
    int producer;

    if (thorium_actor_take_action(self, message)) {
        return;
    }

    concrete_self = (struct bsal_assembly_arc_kernel *)thorium_actor_concrete_actor(self);
    tag = thorium_message_action(message);
    source = thorium_message_source(message);

    if (tag == ACTION_SET_PRODUCER) {

        thorium_message_unpack_int(message, 0, &concrete_self->producer);
        concrete_self->source = source;

        printf("%s/%d received ACTION_SET_PRODUCER, starting now!\n",
                        thorium_actor_script_name(self),
                        thorium_actor_name(self));

        bsal_assembly_arc_kernel_ask(self, message);

    } else if (tag == ACTION_SET_CONSUMER) {

        thorium_message_unpack_int(message, 0, &concrete_self->consumer);

        thorium_actor_send_reply_empty(self, ACTION_SET_CONSUMER_REPLY);

    } else if (tag == ACTION_NOTIFY) {

        thorium_actor_send_reply_uint64_t(self, ACTION_NOTIFY_REPLY,
                        concrete_self->produced_arcs);

    } else if (tag == ACTION_SEQUENCE_STORE_ASK_REPLY) {

        if (bsal_fast_queue_dequeue(&concrete_self->producers_for_work_stealing, &producer)) {

            /*
             * Do some work stealing with the producer of another consumer.
             */
            concrete_self->producer = producer;

            printf("%s/%d will steal work from producer %d now\n",
                            thorium_actor_script_name(self),
                            thorium_actor_name(self),
                            concrete_self->producer);

            bsal_assembly_arc_kernel_ask(self, message);

        } else {

            printf("%s/%d DONE\n",
                            thorium_actor_script_name(self),
                            thorium_actor_name(self));

            thorium_actor_send_empty(self, concrete_self->source,
                        ACTION_SET_PRODUCER_REPLY);
        }
    } else if (tag == ACTION_ASK_TO_STOP) {

        printf("%s/%d generated %" PRIu64 " arcs from %d sequence blocks, generated %d messages for consumer\n",
                        thorium_actor_script_name(self),
                        thorium_actor_name(self),
                        concrete_self->produced_arcs,
                        concrete_self->received_blocks,
                        concrete_self->flushed_messages);

        thorium_actor_ask_to_stop(self, message);

    } else if (tag == ACTION_ASSEMBLY_PUSH_ARC_BLOCK_REPLY) {

        /*
         * Ask for more !
         */
        bsal_assembly_arc_kernel_ask(self, message);
    }
}

void bsal_assembly_arc_kernel_set_kmer_length(struct thorium_actor *self, struct thorium_message *message)
{
    struct bsal_assembly_arc_kernel *concrete_self;

    concrete_self = (struct bsal_assembly_arc_kernel *)thorium_actor_concrete_actor(self);

    thorium_message_unpack_int(message, 0, &concrete_self->kmer_length);

    thorium_actor_send_reply_empty(self, ACTION_SET_KMER_LENGTH_REPLY);
}

void bsal_assembly_arc_kernel_ask(struct thorium_actor *self, struct thorium_message *message)
{
    struct bsal_assembly_arc_kernel *concrete_self;

    concrete_self = (struct bsal_assembly_arc_kernel *)thorium_actor_concrete_actor(self);

    if (concrete_self->consumer == THORIUM_ACTOR_NOBODY) {
        printf("Error: no consumer in arc kernel\n");
        return;
    }

    if (concrete_self->producer == THORIUM_ACTOR_NOBODY) {
        printf("Error: no producer in arc kernel\n");
        return;
    }

    /*
     * Send ACTION_SEQUENCE_STORE_ASK to producer. This message needs
     * a kmer length.
     *
     * There are 2 possible answers:
     *
     * 1. ACTION_SEQUENCE_STORE_ASK_REPLY which means there is nothing available.
     * 2. ACTION_PUSH_SEQUENCE_DATA_BLOCK which contains sequences.
     */

    thorium_actor_send_int(self, concrete_self->producer,
                    ACTION_SEQUENCE_STORE_ASK, concrete_self->kmer_length);
}

void bsal_assembly_arc_kernel_push_sequence_data_block(struct thorium_actor *self, struct thorium_message *message)
{
    struct bsal_assembly_arc_kernel *concrete_self;
    struct bsal_input_command input_block;
    void *buffer;
    struct bsal_memory_pool *ephemeral_memory;
    struct bsal_vector *sequences;
    struct bsal_assembly_arc_block output_block;
    int entries;
    int i;
    struct bsal_dna_sequence *dna_sequence;
    char *sequence;
    int maximum_length;
    int length;
    struct bsal_dna_kmer previous_kmer;
    struct bsal_dna_kmer current_kmer;
    int position;
    char saved;
    char *kmer_sequence;
    int limit;
    int first_symbol;
    int last_symbol;
    struct thorium_message new_message;
    int new_count;
    void *new_buffer;
    int to_reserve;

    ephemeral_memory = thorium_actor_get_ephemeral_memory(self);
    concrete_self = (struct bsal_assembly_arc_kernel *)thorium_actor_concrete_actor(self);
    buffer = thorium_message_buffer(message);

    ++concrete_self->received_blocks;

    if (concrete_self->kmer_length == THORIUM_ACTOR_NOBODY) {
        printf("Error no kmer length set in kernel\n");
        return;
    }

    if (concrete_self->consumer == THORIUM_ACTOR_NOBODY) {
        printf("Error no consumer set in kernel\n");
        return;
    }

    if (concrete_self->source == THORIUM_ACTOR_NOBODY || concrete_self->producer == THORIUM_ACTOR_NOBODY) {
        printf("Error, no producer_source set\n");
        return;
    }

    bsal_input_command_init_empty(&input_block);
    bsal_input_command_unpack(&input_block, buffer, ephemeral_memory,
                    &concrete_self->codec);

    sequences = bsal_input_command_entries(&input_block);

    entries = bsal_vector_size(sequences);

    bsal_assembly_arc_block_init(&output_block, ephemeral_memory,
                    concrete_self->kmer_length, &concrete_self->codec);

    /*
     * Avoid sending repetitions.
     */
    bsal_assembly_arc_block_enable_redundancy_check(&output_block);

    /*
     *
     *
     * Extract arcs from sequences.
     */

    maximum_length = 0;
    to_reserve = 0;

#ifdef DEBUG_MODE_FOR_PAYLOAD
    printf("arc_kernel ACTION_PUSH_SEQUENCE_DATA_BLOCK with %d DNA sequences\n",
                    entries);
#endif

    /*
     * Get maximum length
     */
    for (i = 0 ; i < entries ; i++) {

        dna_sequence = bsal_vector_at(sequences, i);

        length = bsal_dna_sequence_length(dna_sequence);

        if (length > maximum_length) {
            maximum_length = length;
        }

        /*
         * The number of edges is bounded by twice the length
         */
        to_reserve += 2 * length;
    }

    bsal_assembly_arc_block_reserve(&output_block, to_reserve);

    sequence = bsal_memory_pool_allocate(ephemeral_memory, maximum_length + 1);

    /*
     * Generate arcs.
     *
     * This code needs to be fast.
     */
    for (i = 0 ; i < entries ; i++) {

        dna_sequence = bsal_vector_at(sequences, i);
        length = bsal_dna_sequence_length(dna_sequence);
        bsal_dna_sequence_get_sequence(dna_sequence, sequence, &concrete_self->codec);

        limit = length - concrete_self->kmer_length + 1;

        for (position = 0; position < limit; position++) {

            kmer_sequence = sequence + position;
            saved = kmer_sequence[concrete_self->kmer_length];
            kmer_sequence[concrete_self->kmer_length] = '\0';

            bsal_dna_kmer_init(&current_kmer, kmer_sequence, &concrete_self->codec,
                            ephemeral_memory);

            /*
             * Restore the data
             */
            kmer_sequence[concrete_self->kmer_length] = saved;

            /*
             * Is this not the first one ?
             */
            if (position > 0) {

                /*
                 * previous_kmer -> current_kmer (BSAL_ARC_TYPE_CHILD)
                 */

                last_symbol = bsal_dna_kmer_last_symbol(&current_kmer, concrete_self->kmer_length,
                                        &concrete_self->codec);

                bsal_assembly_arc_block_add_arc(&output_block, BSAL_ARC_TYPE_CHILD,
                                &previous_kmer,
                                last_symbol, concrete_self->kmer_length,
                                &concrete_self->codec, ephemeral_memory);
#ifdef BSAL_ASSEMBLY_ADD_ARCS
                ++concrete_self->produced_arcs;
#endif

                /*
                 * previous_kmer -> current_kmer (BSAL_ARC_TYPE_PARENT)
                 */
                first_symbol = bsal_dna_kmer_first_symbol(&previous_kmer, concrete_self->kmer_length,
                                        &concrete_self->codec);

                bsal_assembly_arc_block_add_arc(&output_block, BSAL_ARC_TYPE_PARENT,
                                &current_kmer, first_symbol,
                                concrete_self->kmer_length,
                                &concrete_self->codec, ephemeral_memory);
#ifdef BSAL_ASSEMBLY_ADD_ARCS
                ++concrete_self->produced_arcs;
#endif
            }

            bsal_dna_kmer_init_copy(&previous_kmer, &current_kmer, concrete_self->kmer_length,
                            ephemeral_memory, &concrete_self->codec);

            bsal_dna_kmer_destroy(&current_kmer, ephemeral_memory);

            /* Previous is not needed anymore
             */
            if (position == limit - 1) {

                bsal_dna_kmer_destroy(&previous_kmer, ephemeral_memory);
            }
        }
    }

    new_count = bsal_assembly_arc_block_pack_size(&output_block, concrete_self->kmer_length,
                    &concrete_self->codec);
    new_buffer = bsal_memory_pool_allocate(ephemeral_memory, new_count);

    bsal_assembly_arc_block_pack(&output_block, new_buffer, concrete_self->kmer_length,
                    &concrete_self->codec);

    bsal_assembly_arc_block_destroy(&output_block,
                    ephemeral_memory);

    bsal_input_command_destroy(&input_block, ephemeral_memory);

    thorium_message_init(&new_message, ACTION_ASSEMBLY_PUSH_ARC_BLOCK,
                    new_count, new_buffer);
    thorium_actor_send(self, concrete_self->consumer, &new_message);

    ++concrete_self->flushed_messages;

    thorium_message_destroy(&new_message);

    bsal_memory_pool_free(ephemeral_memory, sequence);
    bsal_memory_pool_free(ephemeral_memory, new_buffer);
}

void bsal_assembly_arc_kernel_set_producers_for_work_stealing(struct thorium_actor *self, struct thorium_message *message)
{
    struct bsal_assembly_arc_kernel *concrete_self;
    struct bsal_memory_pool *ephemeral_memory;
    void *buffer;
    struct bsal_vector producers;
    int i;
    int size;
    int producer;

    ephemeral_memory = thorium_actor_get_ephemeral_memory(self);
    concrete_self = (struct bsal_assembly_arc_kernel *)thorium_actor_concrete_actor(self);
    buffer = thorium_message_buffer(message);

    bsal_vector_init(&producers, sizeof(int));
    bsal_vector_set_memory_pool(&producers, ephemeral_memory);
    bsal_vector_unpack(&producers, buffer);

    i = 0;
    size = bsal_vector_size(&producers);

    while (i < size) {

        producer = bsal_vector_at_as_int(&producers, i);
        bsal_fast_queue_enqueue(&concrete_self->producers_for_work_stealing, &producer);

        ++i;
    }

#if 0
    printf("ACTION_SET_PRODUCERS_FOR_WORK_STEALING: \n");
    bsal_vector_print_int(&producers);
    printf("\n");
#endif

    bsal_vector_destroy(&producers);

    thorium_actor_send_reply_empty(self, ACTION_SET_PRODUCERS_FOR_WORK_STEALING_REPLY);
}
