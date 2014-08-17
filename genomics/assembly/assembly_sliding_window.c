
#include "assembly_sliding_window.h"

#include <genomics/storage/sequence_store.h>

#include <genomics/kernels/aggregator.h>
#include <genomics/kernels/dna_kmer_counter_kernel.h>

#include <genomics/data/dna_kmer.h>
#include <genomics/data/dna_kmer_block.h>
#include <genomics/data/dna_sequence.h>
#include <genomics/input/input_command.h>

#include <core/helpers/message_helper.h>

#include <core/system/packer.h>
#include <core/system/memory.h>
#include <core/system/timer.h>
#include <core/system/debugger.h>

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>

/*
*/

/* debugging options
 */
/*
#define BSAL_WINDOW_DEBUG
*/

/* Disable memory tracking in memory pool
 * for performance purposes.
 */
#define BSAL_ASSEMBLY_SLIDING_WINDOW_DISABLE_TRACKING

#define MAXIMUM_AUTO_SCALING_KERNEL_COUNT 0

struct thorium_script bsal_assembly_sliding_window_script = {
    .identifier = BSAL_ASSEMBLY_SLIDING_WINDOW_SCRIPT,
    .init = bsal_assembly_sliding_window_init,
    .destroy = bsal_assembly_sliding_window_destroy,
    .receive = bsal_assembly_sliding_window_receive,
    .size = sizeof(struct bsal_assembly_sliding_window),
    .name = "bsal_assembly_sliding_window"
};

void bsal_assembly_sliding_window_init(struct thorium_actor *actor)
{
    struct bsal_assembly_sliding_window *concrete_actor;

    concrete_actor = (struct bsal_assembly_sliding_window *)thorium_actor_concrete_actor(actor);

    concrete_actor->expected = 0;
    concrete_actor->actual = 0;
    concrete_actor->last = 0;
    concrete_actor->blocks = 0;

    concrete_actor->kmer_length = -1;
    concrete_actor->consumer = BSAL_ACTOR_NOBODY;
    concrete_actor->producer = BSAL_ACTOR_NOBODY;
    concrete_actor->producer_source = BSAL_ACTOR_NOBODY;

    concrete_actor->notified = 0;
    concrete_actor->notification_source = 0;

    concrete_actor->kmers = 0;

    bsal_dna_codec_init(&concrete_actor->codec);

    if (bsal_dna_codec_must_use_two_bit_encoding(&concrete_actor->codec,
                            thorium_actor_get_node_count(actor))) {
        bsal_dna_codec_enable_two_bit_encoding(&concrete_actor->codec);
    }

    bsal_vector_init(&concrete_actor->kernels, sizeof(int));

    concrete_actor->auto_scaling_in_progress = 0;

    thorium_actor_add_route(actor, BSAL_ACTOR_PACK,
                    bsal_assembly_sliding_window_pack_message);
    thorium_actor_add_route(actor, BSAL_ACTOR_UNPACK,
                    bsal_assembly_sliding_window_unpack_message);
    thorium_actor_add_route(actor, BSAL_ACTOR_CLONE_REPLY,
                    bsal_assembly_sliding_window_clone_reply);
    thorium_actor_add_route(actor, BSAL_ACTOR_NOTIFY,
                    bsal_assembly_sliding_window_notify);
    thorium_actor_add_route(actor, BSAL_ACTOR_NOTIFY_REPLY,
                    bsal_assembly_sliding_window_notify_reply);
    thorium_actor_add_route(actor, BSAL_ACTOR_DO_AUTO_SCALING,
                    bsal_assembly_sliding_window_do_auto_scaling);

    printf("%s/%d is online on node node/%d\n",
                    thorium_actor_script_name(actor),
                    thorium_actor_name(actor),
                    thorium_actor_node_name(actor));

    /* Enable packing for this actor. Maybe this is already enabled, but who knows.
     */

    thorium_actor_send_to_self_empty(actor, BSAL_ACTOR_PACK_ENABLE);
    concrete_actor->auto_scaling_clone = BSAL_ACTOR_NOBODY;

    concrete_actor->scaling_operations = 0;

    concrete_actor->flushed_payloads = 0;
}

void bsal_assembly_sliding_window_destroy(struct thorium_actor *actor)
{
    struct bsal_assembly_sliding_window *concrete_actor;

    concrete_actor = (struct bsal_assembly_sliding_window *)thorium_actor_concrete_actor(actor);

    concrete_actor->consumer = -1;
    concrete_actor->producer = -1;
    concrete_actor->producer_source =-1;

    bsal_dna_codec_destroy(&concrete_actor->codec);
    bsal_vector_destroy(&concrete_actor->kernels);
}

void bsal_assembly_sliding_window_receive(struct thorium_actor *actor, struct thorium_message *message)
{
    int tag;
    int source;
    struct bsal_dna_kmer kmer;
    int name;
    void *buffer;
    struct bsal_assembly_sliding_window *concrete_actor;
    int consumer;
    int count;
    int producer;

    if (thorium_actor_use_route(actor, message)) {
        return;
    }

    count = thorium_message_count(message);

    concrete_actor = (struct bsal_assembly_sliding_window *)thorium_actor_concrete_actor(actor);
    tag = thorium_message_tag(message);
    name = thorium_actor_name(actor);
    source = thorium_message_source(message);
    buffer = thorium_message_buffer(message);

    if (tag == BSAL_PUSH_SEQUENCE_DATA_BLOCK) {
        bsal_assembly_sliding_window_push_sequence_data_block(actor, message);

    } else if (tag == BSAL_AGGREGATE_KERNEL_OUTPUT_REPLY) {

#ifdef BSAL_WINDOW_DEBUG
        BSAL_DEBUG_MARKER("kernel receives reply from aggregator\n");
#endif

        /*
        source_index = *(int *)buffer;
        thorium_actor_send_empty(actor,
                        source_index,
                        BSAL_PUSH_SEQUENCE_DATA_BLOCK_REPLY);
                        */

        bsal_assembly_sliding_window_ask(actor, message);

    } else if (tag == BSAL_ACTOR_START) {

        thorium_actor_send_reply_empty(actor, BSAL_ACTOR_START_REPLY);

    } else if (tag == BSAL_RESERVE) {

        printf("%s/%d is online !\n",
                        thorium_actor_script_name(actor), name);

        concrete_actor->expected = *(uint64_t *)buffer;

        thorium_actor_send_reply_empty(actor, BSAL_RESERVE_REPLY);

    } else if (tag == BSAL_ACTOR_ASK_TO_STOP) {

        printf("window/%d generated %" PRIu64 " kmers from %" PRIu64
                        " entries (%d blocks), sent %d messages to consumer \n",
                        thorium_actor_name(actor), concrete_actor->kmers,
                        concrete_actor->actual, concrete_actor->blocks,
                        concrete_actor->flushed_payloads);

#ifdef BSAL_WINDOW_DEBUG
        printf("window %d receives request to stop from %d, supervisor is %d\n",
                        name, source, thorium_actor_supervisor(actor));
#endif

        /*thorium_actor_send_to_self_empty(actor, BSAL_ACTOR_STOP);*/

        thorium_actor_ask_to_stop(actor, message);

    } else if (tag == BSAL_ACTOR_SET_CONSUMER) {

        consumer = *(int *)buffer;
        concrete_actor->consumer = consumer;

#ifdef BSAL_WINDOW_DEBUG
        printf("window %d BSAL_ACTOR_SET_CONSUMER consumer %d index %d\n",
                        thorium_actor_name(actor), consumer,
                        concrete_actor->consumer);
#endif

        thorium_actor_send_reply_empty(actor, BSAL_ACTOR_SET_CONSUMER_REPLY);

    } else if (tag == BSAL_SET_KMER_LENGTH) {

        thorium_message_unpack_int(message, 0, &concrete_actor->kmer_length);

        printf("%s/%d kmer length is %d\n",
                        thorium_actor_script_name(actor),
                        thorium_actor_name(actor),
                        concrete_actor->kmer_length);

        bsal_dna_kmer_init_mock(&kmer, concrete_actor->kmer_length, &concrete_actor->codec,
                        thorium_actor_get_ephemeral_memory(actor));
        concrete_actor->bytes_per_kmer = bsal_dna_kmer_pack_size(&kmer, concrete_actor->kmer_length,
                        &concrete_actor->codec);
        bsal_dna_kmer_destroy(&kmer, thorium_actor_get_ephemeral_memory(actor));

        thorium_actor_send_reply_empty(actor, BSAL_SET_KMER_LENGTH_REPLY);

    } else if (tag == BSAL_ACTOR_SET_PRODUCER) {

        if (count == 0) {
            printf("Error: window needs producer\n");
            return;
        }

        if (concrete_actor->consumer == BSAL_ACTOR_NOBODY) {
            printf("Error: window needs a consumer\n");
            return;
        }

        if (concrete_actor->kmer_length <= 0) {

            printf("%s/%d Error: invalid kmer length, %d\n",
                            thorium_actor_script_name(actor),
                            thorium_actor_name(actor),
                            concrete_actor->kmer_length);
            return;
        }

        thorium_message_unpack_int(message, 0, &producer);

        concrete_actor->producer = producer;

        bsal_assembly_sliding_window_ask(actor, message);

        concrete_actor->producer_source = source;

    } else if (tag == BSAL_SEQUENCE_STORE_ASK_REPLY) {

        /* the store has no more sequence...
         */

#ifdef BSAL_ASSEMBLY_SLIDING_WINDOW_DEBUG
        printf("DEBUG window was told by producer that nothing is left to do\n");
#endif

        thorium_actor_send_empty(actor,
                                concrete_actor->producer_source,
                        BSAL_ACTOR_SET_PRODUCER_REPLY);

    } else if (tag == BSAL_ACTOR_SET_CONSUMER_REPLY) {

        /* Set the producer for the new window
         */
        producer = concrete_actor->producer;

        thorium_actor_send_int(actor, concrete_actor->auto_scaling_clone,
                        BSAL_ACTOR_SET_PRODUCER, producer);

        concrete_actor->auto_scaling_in_progress = 0;
        concrete_actor->scaling_operations++;
        concrete_actor->auto_scaling_clone = BSAL_ACTOR_NOBODY;

        printf("window %d completed auto-scaling # %d\n",
                        thorium_actor_name(actor),
                        concrete_actor->scaling_operations);



    } else if (tag == BSAL_ACTOR_SET_PRODUCER_REPLY
                    && source == concrete_actor->auto_scaling_clone) {

    } else if (tag == BSAL_ACTOR_ENABLE_AUTO_SCALING) {

        printf("AUTO-SCALING window %d enables auto-scaling (BSAL_ACTOR_ENABLE_AUTO_SCALING) via actor %d\n",
                        name, source);

        thorium_actor_send_to_self_empty(actor, BSAL_ACTOR_ENABLE_AUTO_SCALING);

    }
}

void bsal_assembly_sliding_window_verify(struct thorium_actor *actor, struct thorium_message *message)
{
    struct bsal_assembly_sliding_window *concrete_actor;

    concrete_actor = (struct bsal_assembly_sliding_window *)thorium_actor_concrete_actor(actor);

    if (!concrete_actor->notified) {

        return;
    }

#if 0
    if (concrete_actor->actual != concrete_actor->expected) {
        return;
    }
#endif

    thorium_actor_send_uint64_t(actor,
                            concrete_actor->notification_source,
                    BSAL_ACTOR_NOTIFY_REPLY, concrete_actor->kmers);
}

void bsal_assembly_sliding_window_ask(struct thorium_actor *self, struct thorium_message *message)
{
    struct bsal_assembly_sliding_window *concrete_actor;
    int producer;

    concrete_actor = (struct bsal_assembly_sliding_window *)thorium_actor_concrete_actor(self);

    producer = concrete_actor->producer;

    thorium_actor_send_int(self, producer, BSAL_SEQUENCE_STORE_ASK,
                    concrete_actor->kmer_length);

#ifdef BSAL_ASSEMBLY_SLIDING_WINDOW_DEBUG
    printf("DEBUG window asks producer\n");
#endif
}

void bsal_assembly_sliding_window_do_auto_scaling(struct thorium_actor *actor, struct thorium_message *message)
{
    struct bsal_assembly_sliding_window *concrete_actor;
    int name;
    int source;

    name = thorium_actor_name(actor);
    source = thorium_message_source(message);

    concrete_actor = (struct bsal_assembly_sliding_window *)thorium_actor_concrete_actor(actor);

    /*
     * Don't do auto-scaling while doing auto-scaling...
     */
    if (concrete_actor->auto_scaling_in_progress) {
        return;
    }

    /*
     * Avoid cloning too many actors
     */
    if (concrete_actor->scaling_operations >= MAXIMUM_AUTO_SCALING_KERNEL_COUNT) {
        return;
    }

    /*
     * Can't scale now because there is no producer
     */
    if (concrete_actor->producer == BSAL_ACTOR_NOBODY) {
        return;
    }

    /* - spawn a kernel
     * - spawn an aggregator
     * - set the aggregator as the consumer of the kernel
     * - set the kmer stores as the consumers of the aggregator
     * - set a sequence store as the producer of the kernel (BSAL_ACTOR_SET_PRODUCER)
     */

    printf("AUTO-SCALING window %d receives auto-scale message (BSAL_ACTOR_DO_AUTO_SCALING) via actor %d\n",
                    name, source);

    concrete_actor->auto_scaling_in_progress = 1;

    thorium_actor_send_to_self_int(actor, BSAL_ACTOR_CLONE, name);
}

void bsal_assembly_sliding_window_pack_message(struct thorium_actor *actor, struct thorium_message *message)
{
    int *new_buffer;
    int new_count;
    struct thorium_message new_message;
    struct bsal_memory_pool *ephemeral_memory;

    ephemeral_memory = thorium_actor_get_ephemeral_memory(actor);
    new_count = bsal_assembly_sliding_window_pack_size(actor);
    new_buffer = bsal_memory_pool_allocate(ephemeral_memory, new_count);

    bsal_assembly_sliding_window_pack(actor, new_buffer);

    thorium_message_init(&new_message, BSAL_ACTOR_PACK_REPLY, new_count, new_buffer);

    thorium_actor_send_reply(actor, &new_message);

    thorium_message_destroy(&new_message);
    bsal_memory_pool_free(ephemeral_memory, new_buffer);
    new_buffer = NULL;
}

void bsal_assembly_sliding_window_unpack_message(struct thorium_actor *actor, struct thorium_message *message)
{
    void *buffer;

    buffer = thorium_message_buffer(message);

    bsal_assembly_sliding_window_unpack(actor, buffer);

    thorium_actor_send_reply_empty(actor, BSAL_ACTOR_UNPACK_REPLY);
}

void bsal_assembly_sliding_window_clone_reply(struct thorium_actor *actor, struct thorium_message *message)
{
    struct bsal_assembly_sliding_window *concrete_actor;
    int name;
    int clone;
    int source;
    int consumer;
    /*int producer;*/
    int clone_index;

    source = thorium_message_source(message);
    concrete_actor = (struct bsal_assembly_sliding_window *)thorium_actor_concrete_actor(actor);
    name = thorium_actor_name(actor);
    thorium_message_unpack_int(message, 0, &clone);
    consumer = concrete_actor->consumer;
    /*producer = concrete_actor->producer);*/

    if (source == name) {
        printf("window %d cloned itself !!! clone name is %d\n",
                    name, clone);

        thorium_actor_send_int(actor, consumer, BSAL_ACTOR_CLONE, name);

        concrete_actor->auto_scaling_clone = clone;

        clone_index = clone;

        bsal_vector_push_back(&concrete_actor->kernels, &clone_index);

    } else if (source == consumer) {
        printf("window %d cloned aggregator %d, clone name is %d\n",
                        name, consumer, clone);

        thorium_actor_send_int(actor, concrete_actor->auto_scaling_clone,
                        BSAL_ACTOR_SET_CONSUMER, clone);

    }
}


int bsal_assembly_sliding_window_pack(struct thorium_actor *actor, void *buffer)
{
    return bsal_assembly_sliding_window_pack_unpack(actor, BSAL_PACKER_OPERATION_PACK, buffer);
}

int bsal_assembly_sliding_window_unpack(struct thorium_actor *actor, void *buffer)
{
    return bsal_assembly_sliding_window_pack_unpack(actor, BSAL_PACKER_OPERATION_UNPACK, buffer);
}

int bsal_assembly_sliding_window_pack_size(struct thorium_actor *actor)
{
    return bsal_assembly_sliding_window_pack_unpack(actor, BSAL_PACKER_OPERATION_DRY_RUN, NULL);
}

/*
 * Copy these:
 *
 * - kmer length
 * - consumer
 * - producer
 */
int bsal_assembly_sliding_window_pack_unpack(struct thorium_actor *actor, int operation, void *buffer)
{
    int bytes;
    struct bsal_packer packer;
    struct bsal_assembly_sliding_window *concrete_actor;
    int producer;
    int consumer;

    concrete_actor = (struct bsal_assembly_sliding_window *)thorium_actor_concrete_actor(actor);
    producer = BSAL_ACTOR_NOBODY;
    consumer = BSAL_ACTOR_NOBODY;

    if (operation != BSAL_PACKER_OPERATION_UNPACK) {
        producer = concrete_actor->producer;
        consumer = concrete_actor->consumer;
    }

    bytes = 0;

    bsal_packer_init(&packer, operation, buffer);

    bsal_packer_work(&packer, &concrete_actor->kmer_length, sizeof(concrete_actor->kmer_length));
    bsal_packer_work(&packer, &producer, sizeof(producer));
    bsal_packer_work(&packer, &consumer, sizeof(consumer));

    if (operation == BSAL_PACKER_OPERATION_UNPACK) {

        concrete_actor->producer = producer;
        concrete_actor->consumer = consumer;
    }

    bytes += bsal_packer_worked_bytes(&packer);
    bsal_packer_destroy(&packer);

    return bytes;
}

void bsal_assembly_sliding_window_notify(struct thorium_actor *actor, struct thorium_message *message)
{
    struct bsal_assembly_sliding_window *concrete_actor;
    struct bsal_vector_iterator iterator;
    int kernel;
    int source;

    concrete_actor = (struct bsal_assembly_sliding_window *)thorium_actor_concrete_actor(actor);

    source = thorium_message_source(message);
    concrete_actor->notified = 1;

    thorium_actor_send_to_self_empty(actor, BSAL_ACTOR_DISABLE_AUTO_SCALING);

    concrete_actor->notification_source = source;

    if (concrete_actor->scaling_operations > 0) {

        concrete_actor->sum_of_kmers = 0;
        concrete_actor->sum_of_kmers += concrete_actor->kmers;
        concrete_actor->notified_children = 0;

        bsal_vector_iterator_init(&iterator, &concrete_actor->kernels);

        while (bsal_vector_iterator_get_next_value(&iterator, &kernel)) {

            thorium_actor_send_empty(actor, kernel, BSAL_ACTOR_NOTIFY);
        }

        bsal_vector_iterator_destroy(&iterator);


    } else {
        bsal_assembly_sliding_window_verify(actor, message);
    }
}

void bsal_assembly_sliding_window_notify_reply(struct thorium_actor *actor, struct thorium_message *message)
{
    struct bsal_assembly_sliding_window *concrete_actor;
    uint64_t kmers;

    concrete_actor = (struct bsal_assembly_sliding_window *)thorium_actor_concrete_actor(actor);

    thorium_message_unpack_uint64_t(message, 0, &kmers);

    concrete_actor->sum_of_kmers += kmers;

    concrete_actor->notified_children++;

    if (concrete_actor->notified_children == bsal_vector_size(&concrete_actor->kernels)) {


        thorium_actor_send_uint64_t(actor,
                            concrete_actor->notification_source,
                    BSAL_ACTOR_NOTIFY_REPLY, concrete_actor->sum_of_kmers);

    }

}

void bsal_assembly_sliding_window_push_sequence_data_block(struct thorium_actor *actor, struct thorium_message *message)
{
    int source;
    struct bsal_dna_kmer kmer;
    int name;
    struct bsal_input_command payload;
    void *buffer;
    int entries;
    struct bsal_assembly_sliding_window *concrete_actor;
    int source_index;
    int consumer;
    int i;
    struct bsal_dna_sequence *sequence;
    char *sequence_data;
    struct bsal_vector *command_entries;
    int sequence_length;
    int new_count;
    void *new_buffer;
    struct thorium_message new_message;
    int j;
    int limit;
    char saved;
    struct bsal_timer timer;
    struct bsal_dna_kmer_block block;
    int to_reserve;
    int maximum_length;
    struct bsal_memory_pool *ephemeral_memory;
    int kmers_for_sequence;

    concrete_actor = (struct bsal_assembly_sliding_window *)thorium_actor_concrete_actor(actor);
    ephemeral_memory = thorium_actor_get_ephemeral_memory(actor);
    name = thorium_actor_name(actor);
    source = thorium_message_source(message);
    buffer = thorium_message_buffer(message);

    if (concrete_actor->kmer_length == BSAL_ACTOR_NOBODY) {
        printf("Error no kmer length set\n");
        return;
    }

    if (concrete_actor->consumer == BSAL_ACTOR_NOBODY) {
        printf("Error no consumer set\n");
        return;
    }

    if (concrete_actor->producer_source == BSAL_ACTOR_NOBODY) {
        printf("Error, no producer_source set\n");
        return;
    }

    bsal_timer_init(&timer);
    bsal_timer_start(&timer);

    consumer = concrete_actor->consumer;
    source_index = source;

    bsal_input_command_init_empty(&payload);
    bsal_input_command_unpack(&payload, buffer, thorium_actor_get_ephemeral_memory(actor),
                    &concrete_actor->codec);

    command_entries = bsal_input_command_entries(&payload);

    entries = bsal_vector_size(command_entries);

    if (entries == 0) {
        printf("Error: received empty payload...\n");
    }

    to_reserve = 0;
    maximum_length = 0;

    for (i = 0; i < entries; i++) {

        sequence = (struct bsal_dna_sequence *)bsal_vector_at(command_entries, i);

        sequence_length = bsal_dna_sequence_length(sequence);

        if (sequence_length > maximum_length) {
            maximum_length = sequence_length;
        }

        to_reserve += (sequence_length - concrete_actor->kmer_length + 1);
    }

    bsal_dna_kmer_block_init(&block, concrete_actor->kmer_length, source_index, to_reserve);

    sequence_data = bsal_memory_pool_allocate(ephemeral_memory, maximum_length + 1);

    /* extract kmers
     */
    for (i = 0; i < entries; i++) {

        sequence = (struct bsal_dna_sequence *)bsal_vector_at(command_entries, i);

        bsal_dna_sequence_get_sequence(sequence, sequence_data,
                        &concrete_actor->codec);

        sequence_length = bsal_dna_sequence_length(sequence);
        limit = sequence_length - concrete_actor->kmer_length + 1;

        kmers_for_sequence = 0;

        for (j = 0; j < limit; j++) {
            saved = sequence_data[j + concrete_actor->kmer_length];
            sequence_data[j + concrete_actor->kmer_length] = '\0';

            bsal_dna_kmer_init(&kmer, sequence_data + j,
                            &concrete_actor->codec, thorium_actor_get_ephemeral_memory(actor));

#ifdef BSAL_WINDOW_DEBUG_LEVEL_2
            printf("KERNEL kmer %d,%d %s\n", i, j, sequence_data + j);
#endif

            /*
             * add kmer in block
             */
            bsal_dna_kmer_block_add_kmer(&block, &kmer, thorium_actor_get_ephemeral_memory(actor),
                            &concrete_actor->codec);

            bsal_dna_kmer_destroy(&kmer, thorium_actor_get_ephemeral_memory(actor));

            sequence_data[j + concrete_actor->kmer_length] = saved;

            ++kmers_for_sequence;
        }

#ifdef BSAL_PRIVATE_DEBUG_EMIT
        printf("DEBUG EMIT KMERS INPUT: %d nucleotides, k: %d output %d kmers\n",
                        sequence_length, concrete_actor->kmer_length,
                        kmers_for_sequence);
#endif

        concrete_actor->kmers += kmers_for_sequence;
    }

    bsal_memory_pool_free(ephemeral_memory, sequence_data);
    sequence_data = NULL;

#ifdef BSAL_WINDOW_DEBUG
    BSAL_DEBUG_MARKER("after generating kmers\n");
#endif

    concrete_actor->actual += entries;
    concrete_actor->blocks++;

    bsal_input_command_destroy(&payload, thorium_actor_get_ephemeral_memory(actor));

    new_count = bsal_dna_kmer_block_pack_size(&block,
                    &concrete_actor->codec);
    new_buffer = bsal_memory_pool_allocate(ephemeral_memory, new_count);
    bsal_dna_kmer_block_pack(&block, new_buffer,
                        &concrete_actor->codec);

#ifdef BSAL_WINDOW_DEBUG
    printf("name %d destination %d PACK with %d bytes\n", name,
                       consumer, new_count);
#endif


    thorium_message_init(&new_message, BSAL_AGGREGATE_KERNEL_OUTPUT,
                    new_count, new_buffer);

    /*
    thorium_message_init(&new_message, BSAL_AGGREGATE_KERNEL_OUTPUT,
                    sizeof(source_index), &source_index);
                    */

    thorium_actor_send(actor, consumer, &new_message);
    bsal_memory_pool_free(ephemeral_memory, new_buffer);

    ++concrete_actor->flushed_payloads;

    thorium_actor_send_empty(actor,
                    source_index,
                    BSAL_PUSH_SEQUENCE_DATA_BLOCK_REPLY);

    if (concrete_actor->actual == concrete_actor->expected
                    || concrete_actor->actual >= concrete_actor->last + 300000
                    || concrete_actor->last == 0) {

        printf("sliding window %d processed %" PRIu64 " entries (%d blocks) so far\n",
                        name, concrete_actor->actual,
                        concrete_actor->blocks);

        concrete_actor->last = concrete_actor->actual;
    }

    bsal_timer_stop(&timer);

#ifdef BSAL_WINDOW_DEBUG

        bsal_timer_print(&timer);
#endif

    bsal_timer_destroy(&timer);

    bsal_dna_kmer_block_destroy(&block, thorium_actor_get_ephemeral_memory(actor));

#ifdef BSAL_WINDOW_DEBUG
    BSAL_DEBUG_MARKER("leaving call.\n");
#endif

    bsal_assembly_sliding_window_verify(actor, message);

}
