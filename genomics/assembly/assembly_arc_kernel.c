
#include "assembly_arc_kernel.h"

#include "assembly_arc_block.h"

#include <genomics/kernels/dna_kmer_counter_kernel.h>

#include <genomics/input/input_command.h>

#include <genomics/storage/sequence_store.h>

#include <stdio.h>

#include <stdint.h>
#include <inttypes.h>

struct bsal_script bsal_assembly_arc_kernel_script = {
    .identifier = BSAL_ASSEMBLY_ARC_KERNEL_SCRIPT,
    .name = "bsal_assembly_arc_kernel",
    .init = bsal_assembly_arc_kernel_init,
    .destroy = bsal_assembly_arc_kernel_destroy,
    .receive = bsal_assembly_arc_kernel_receive,
    .size = sizeof(struct bsal_assembly_arc_kernel)
};

void bsal_assembly_arc_kernel_init(struct bsal_actor *self)
{
    struct bsal_assembly_arc_kernel *concrete_self;

    concrete_self = (struct bsal_assembly_arc_kernel *)bsal_actor_concrete_actor(self);

    concrete_self->kmer_length = -1;

    bsal_actor_add_route(self, BSAL_SET_KMER_LENGTH,
                    bsal_assembly_arc_kernel_set_kmer_length);

    printf("%s/%d is now active\n",
                    bsal_actor_script_name(self),
                    bsal_actor_name(self));

    concrete_self->producer = BSAL_ACTOR_NOBODY;
    concrete_self->consumer = BSAL_ACTOR_NOBODY;

    /*
     * Configure the codec.
     */

    bsal_dna_codec_init(&concrete_self->codec);

    if (bsal_actor_get_node_count(self) >= BSAL_DNA_CODEC_MINIMUM_NODE_COUNT_FOR_TWO_BIT) {
#ifdef BSAL_DNA_CODEC_USE_TWO_BIT_ENCODING_FOR_TRANSPORT
        bsal_dna_codec_enable_two_bit_encoding(&concrete_self->codec);
#endif
    }

    concrete_self->produced_arcs = 0;

    bsal_actor_add_route(self, BSAL_PUSH_SEQUENCE_DATA_BLOCK,
                    bsal_assembly_arc_kernel_push_sequence_data_block);

    concrete_self->received_blocks = 0;
}

void bsal_assembly_arc_kernel_destroy(struct bsal_actor *self)
{
    struct bsal_assembly_arc_kernel *concrete_self;

    concrete_self = (struct bsal_assembly_arc_kernel *)bsal_actor_concrete_actor(self);

    concrete_self->kmer_length = -1;

    bsal_dna_codec_destroy(&concrete_self->codec);

    concrete_self->producer = BSAL_ACTOR_NOBODY;
    concrete_self->consumer = BSAL_ACTOR_NOBODY;
}

void bsal_assembly_arc_kernel_receive(struct bsal_actor *self, struct bsal_message *message)
{
    int tag;
    int source;
    struct bsal_assembly_arc_kernel *concrete_self;

    if (bsal_actor_use_route(self, message)) {
        return;
    }

    concrete_self = (struct bsal_assembly_arc_kernel *)bsal_actor_concrete_actor(self);
    tag = bsal_message_tag(message);
    source = bsal_message_source(message);

    if (tag == BSAL_ACTOR_SET_PRODUCER) {

        bsal_message_unpack_int(message, 0, &concrete_self->producer);

        concrete_self->source = source;

        bsal_assembly_arc_kernel_ask(self, message);

    } else if (tag == BSAL_ACTOR_SET_CONSUMER) {

        bsal_message_unpack_int(message, 0, &concrete_self->consumer);

        bsal_actor_send_reply_empty(self, BSAL_ACTOR_SET_CONSUMER_REPLY);

    } else if (tag == BSAL_ACTOR_NOTIFY) {

        bsal_actor_send_reply_uint64_t(self, BSAL_ACTOR_NOTIFY_REPLY,
                        concrete_self->produced_arcs);

    } else if (tag == BSAL_SEQUENCE_STORE_ASK_REPLY) {

        bsal_actor_send_empty(self, concrete_self->source,
                        BSAL_ACTOR_SET_PRODUCER_REPLY);

    } else if (tag == BSAL_ACTOR_ASK_TO_STOP) {

        printf("%s/%d generated %" PRIu64 " arcs from %d sequence blocks\n",
                        bsal_actor_script_name(self),
                        bsal_actor_name(self),
                        concrete_self->produced_arcs,
                        concrete_self->received_blocks);

        bsal_actor_ask_to_stop(self, message);

    } else if (tag == BSAL_ASSEMBLY_PUSH_ARC_BLOCK_REPLY) {

        /*
         * Ask for more !
         */
        bsal_assembly_arc_kernel_ask(self, message);
    }
}

void bsal_assembly_arc_kernel_set_kmer_length(struct bsal_actor *self, struct bsal_message *message)
{
    struct bsal_assembly_arc_kernel *concrete_self;

    concrete_self = (struct bsal_assembly_arc_kernel *)bsal_actor_concrete_actor(self);

    bsal_message_unpack_int(message, 0, &concrete_self->kmer_length);

    bsal_actor_send_reply_empty(self, BSAL_SET_KMER_LENGTH_REPLY);
}

void bsal_assembly_arc_kernel_ask(struct bsal_actor *self, struct bsal_message *message)
{
    struct bsal_assembly_arc_kernel *concrete_self;

    concrete_self = (struct bsal_assembly_arc_kernel *)bsal_actor_concrete_actor(self);

    if (concrete_self->consumer == BSAL_ACTOR_NOBODY) {
        printf("Error: no consumer in arc kernel\n");
        return;
    }

    if (concrete_self->producer == BSAL_ACTOR_NOBODY) {
        printf("Error: no producer in arc kernel\n");
        return;
    }

    /*
     * Send BSAL_SEQUENCE_STORE_ASK to producer. This message needs
     * a kmer length.
     *
     * There are 2 possible answers:
     *
     * 1. BSAL_SEQUENCE_STORE_ASK_REPLY which means there is nothing available.
     * 2. BSAL_PUSH_SEQUENCE_DATA_BLOCK which contains sequences.
     */

    bsal_actor_send_int(self, concrete_self->producer,
                    BSAL_SEQUENCE_STORE_ASK, concrete_self->kmer_length);
}

void bsal_assembly_arc_kernel_push_sequence_data_block(struct bsal_actor *self, struct bsal_message *message)
{
    struct bsal_assembly_arc_kernel *concrete_self;
    struct bsal_input_command input_block;
    void *buffer;
    struct bsal_memory_pool *ephemeral_memory;
    struct bsal_vector *command_entries;
    struct bsal_assembly_arc_block output_block;
    int entries;
    int i;

    ephemeral_memory = bsal_actor_get_ephemeral_memory(self);
    concrete_self = (struct bsal_assembly_arc_kernel *)bsal_actor_concrete_actor(self);
    buffer = bsal_message_buffer(message);

    ++concrete_self->received_blocks;

    if (concrete_self->kmer_length == BSAL_ACTOR_NOBODY) {
        printf("Error no kmer length set in kernel\n");
        return;
    }

    if (concrete_self->consumer == BSAL_ACTOR_NOBODY) {
        printf("Error no consumer set in kernel\n");
        return;
    }

    if (concrete_self->source == BSAL_ACTOR_NOBODY || concrete_self->producer == BSAL_ACTOR_NOBODY) {
        printf("Error, no producer_source set\n");
        return;
    }

    bsal_input_command_unpack(&input_block, buffer, ephemeral_memory,
                    &concrete_self->codec);

    command_entries = bsal_input_command_entries(&input_block);

    entries = bsal_vector_size(command_entries);

    bsal_assembly_arc_block_init(&output_block, ephemeral_memory,
                    concrete_self->kmer_length, &concrete_self->codec);

    /*
     * TODO
     *
     * Extract arcs from sequences.
     */

    for (i = 0 ; i < entries ; i++) {

    }

    bsal_assembly_arc_block_destroy(&output_block,
                    ephemeral_memory);

    bsal_actor_send_empty(self, concrete_self->consumer,
                    BSAL_ASSEMBLY_PUSH_ARC_BLOCK);
}


