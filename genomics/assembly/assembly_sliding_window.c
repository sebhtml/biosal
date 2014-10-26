
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
#define BIOSAL_WINDOW_DEBUG
*/

/* Disable memory tracking in memory pool
 * for performance purposes.
 */
#define BIOSAL_ASSEMBLY_SLIDING_WINDOW_DISABLE_TRACKING

#define MAXIMUM_AUTO_SCALING_KERNEL_COUNT 0

void biosal_assembly_sliding_window_init(struct thorium_actor *actor);
void biosal_assembly_sliding_window_destroy(struct thorium_actor *actor);
void biosal_assembly_sliding_window_receive(struct thorium_actor *actor, struct thorium_message *message);

void biosal_assembly_sliding_window_verify(struct thorium_actor *actor, struct thorium_message *message);
void biosal_assembly_sliding_window_ask(struct thorium_actor *self, struct thorium_message *message);

void biosal_assembly_sliding_window_do_auto_scaling(struct thorium_actor *self, struct thorium_message *message);

void biosal_assembly_sliding_window_pack_message(struct thorium_actor *actor, struct thorium_message *message);
void biosal_assembly_sliding_window_unpack_message(struct thorium_actor *actor, struct thorium_message *message);
void biosal_assembly_sliding_window_clone_reply(struct thorium_actor *actor, struct thorium_message *message);

int biosal_assembly_sliding_window_pack(struct thorium_actor *actor, void *buffer);
int biosal_assembly_sliding_window_unpack(struct thorium_actor *actor, void *buffer);
int biosal_assembly_sliding_window_pack_size(struct thorium_actor *actor);
int biosal_assembly_sliding_window_pack_unpack(struct thorium_actor *actor, int operation, void *buffer);

void biosal_assembly_sliding_window_notify(struct thorium_actor *actor, struct thorium_message *message);
void biosal_assembly_sliding_window_notify_reply(struct thorium_actor *actor, struct thorium_message *message);
void biosal_assembly_sliding_window_push_sequence_data_block(struct thorium_actor *actor, struct thorium_message *message);

void biosal_assembly_sliding_window_set_producers_for_work_stealing(struct thorium_actor *self, struct thorium_message *message);

struct thorium_script biosal_assembly_sliding_window_script = {
    .identifier = SCRIPT_ASSEMBLY_SLIDING_WINDOW,
    .init = biosal_assembly_sliding_window_init,
    .destroy = biosal_assembly_sliding_window_destroy,
    .receive = biosal_assembly_sliding_window_receive,
    .size = sizeof(struct biosal_assembly_sliding_window),
    .name = "biosal_assembly_sliding_window"
};

void biosal_assembly_sliding_window_init(struct thorium_actor *actor)
{
    struct biosal_assembly_sliding_window *concrete_actor;

    concrete_actor = (struct biosal_assembly_sliding_window *)thorium_actor_concrete_actor(actor);

    concrete_actor->expected = 0;
    concrete_actor->actual = 0;
    concrete_actor->last = 0;
    concrete_actor->blocks = 0;

    core_fast_queue_init(&concrete_actor->producers_for_work_stealing, sizeof(int));

    concrete_actor->kmer_length = -1;
    concrete_actor->consumer = THORIUM_ACTOR_NOBODY;
    concrete_actor->producer = THORIUM_ACTOR_NOBODY;
    concrete_actor->producer_source = THORIUM_ACTOR_NOBODY;

    concrete_actor->notified = 0;
    concrete_actor->notification_source = 0;

    concrete_actor->kmers = 0;

    biosal_dna_codec_init(&concrete_actor->codec);

    if (biosal_dna_codec_must_use_two_bit_encoding(&concrete_actor->codec,
                            thorium_actor_get_node_count(actor))) {
        biosal_dna_codec_enable_two_bit_encoding(&concrete_actor->codec);
    }

    core_vector_init(&concrete_actor->kernels, sizeof(int));

    concrete_actor->auto_scaling_in_progress = 0;

    thorium_actor_add_action(actor, ACTION_PACK,
                    biosal_assembly_sliding_window_pack_message);
    thorium_actor_add_action(actor, ACTION_UNPACK,
                    biosal_assembly_sliding_window_unpack_message);
    thorium_actor_add_action(actor, ACTION_CLONE_REPLY,
                    biosal_assembly_sliding_window_clone_reply);
    thorium_actor_add_action(actor, ACTION_NOTIFY,
                    biosal_assembly_sliding_window_notify);
    thorium_actor_add_action(actor, ACTION_NOTIFY_REPLY,
                    biosal_assembly_sliding_window_notify_reply);
    thorium_actor_add_action(actor, ACTION_DO_AUTO_SCALING,
                    biosal_assembly_sliding_window_do_auto_scaling);
    thorium_actor_add_action(actor, ACTION_SET_PRODUCERS_FOR_WORK_STEALING,
                    biosal_assembly_sliding_window_set_producers_for_work_stealing);

    printf("%s/%d is online on node node/%d\n",
                    thorium_actor_script_name(actor),
                    thorium_actor_name(actor),
                    thorium_actor_node_name(actor));

    /* Enable packing for this actor. Maybe this is already enabled, but who knows.
     */

    thorium_actor_send_to_self_empty(actor, ACTION_PACK_ENABLE);
    concrete_actor->auto_scaling_clone = THORIUM_ACTOR_NOBODY;

    concrete_actor->scaling_operations = 0;

    concrete_actor->flushed_payloads = 0;
}

void biosal_assembly_sliding_window_destroy(struct thorium_actor *actor)
{
    struct biosal_assembly_sliding_window *concrete_actor;

    concrete_actor = (struct biosal_assembly_sliding_window *)thorium_actor_concrete_actor(actor);

    concrete_actor->consumer = -1;
    concrete_actor->producer = -1;
    concrete_actor->producer_source =-1;

    biosal_dna_codec_destroy(&concrete_actor->codec);
    core_vector_destroy(&concrete_actor->kernels);

    core_fast_queue_destroy(&concrete_actor->producers_for_work_stealing);
}

void biosal_assembly_sliding_window_receive(struct thorium_actor *actor, struct thorium_message *message)
{
    int tag;
    int source;
    struct biosal_dna_kmer kmer;
    int name;
    void *buffer;
    struct biosal_assembly_sliding_window *concrete_actor;
    int consumer;
    int count;
    int producer;

    if (thorium_actor_take_action(actor, message)) {
        return;
    }

    count = thorium_message_count(message);

    concrete_actor = (struct biosal_assembly_sliding_window *)thorium_actor_concrete_actor(actor);
    tag = thorium_message_action(message);
    name = thorium_actor_name(actor);
    source = thorium_message_source(message);
    buffer = thorium_message_buffer(message);

    if (tag == ACTION_PUSH_SEQUENCE_DATA_BLOCK) {
        biosal_assembly_sliding_window_push_sequence_data_block(actor, message);

    } else if (tag == ACTION_AGGREGATE_KERNEL_OUTPUT_REPLY) {

#ifdef BIOSAL_WINDOW_DEBUG
        BIOSAL_DEBUG_MARKER("kernel receives reply from aggregator\n");
#endif

        /*
        source_index = *(int *)buffer;
        thorium_actor_send_empty(actor,
                        source_index,
                        ACTION_PUSH_SEQUENCE_DATA_BLOCK_REPLY);
                        */

        biosal_assembly_sliding_window_ask(actor, message);

    } else if (tag == ACTION_START) {

        thorium_actor_send_reply_empty(actor, ACTION_START_REPLY);

    } else if (tag == ACTION_RESERVE) {

        printf("%s/%d is online !\n",
                        thorium_actor_script_name(actor), name);

        concrete_actor->expected = *(uint64_t *)buffer;

        thorium_actor_send_reply_empty(actor, ACTION_RESERVE_REPLY);

    } else if (tag == ACTION_ASK_TO_STOP) {

        printf("window/%d generated %" PRIu64 " kmers from %" PRIu64
                        " entries (%d blocks), sent %d messages to consumer \n",
                        thorium_actor_name(actor), concrete_actor->kmers,
                        concrete_actor->actual, concrete_actor->blocks,
                        concrete_actor->flushed_payloads);

#ifdef BIOSAL_WINDOW_DEBUG
        printf("window %d receives request to stop from %d, supervisor is %d\n",
                        name, source, thorium_actor_supervisor(actor));
#endif

        /*thorium_actor_send_to_self_empty(actor, ACTION_STOP);*/

        thorium_actor_ask_to_stop(actor, message);

    } else if (tag == ACTION_SET_CONSUMER) {

        consumer = *(int *)buffer;
        concrete_actor->consumer = consumer;

#ifdef BIOSAL_WINDOW_DEBUG
        printf("window %d ACTION_SET_CONSUMER consumer %d index %d\n",
                        thorium_actor_name(actor), consumer,
                        concrete_actor->consumer);
#endif

        thorium_actor_send_reply_empty(actor, ACTION_SET_CONSUMER_REPLY);

    } else if (tag == ACTION_SET_KMER_LENGTH) {

        thorium_message_unpack_int(message, 0, &concrete_actor->kmer_length);

        printf("%s/%d kmer length is %d\n",
                        thorium_actor_script_name(actor),
                        thorium_actor_name(actor),
                        concrete_actor->kmer_length);

        biosal_dna_kmer_init_mock(&kmer, concrete_actor->kmer_length, &concrete_actor->codec,
                        thorium_actor_get_ephemeral_memory(actor));
        concrete_actor->bytes_per_kmer = biosal_dna_kmer_pack_size(&kmer, concrete_actor->kmer_length,
                        &concrete_actor->codec);
        biosal_dna_kmer_destroy(&kmer, thorium_actor_get_ephemeral_memory(actor));

        thorium_actor_send_reply_empty(actor, ACTION_SET_KMER_LENGTH_REPLY);

    } else if (tag == ACTION_SET_PRODUCER) {

        if (count == 0) {
            printf("Error: window needs producer\n");
            return;
        }

        if (concrete_actor->consumer == THORIUM_ACTOR_NOBODY) {
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

        biosal_assembly_sliding_window_ask(actor, message);

        concrete_actor->producer_source = source;

    } else if (tag == ACTION_SEQUENCE_STORE_ASK_REPLY) {

        /* the store has no more sequence...
         */

#ifdef BIOSAL_ASSEMBLY_SLIDING_WINDOW_DEBUG
        printf("DEBUG window was told by producer that nothing is left to do\n");
#endif

        if (core_fast_queue_dequeue(&concrete_actor->producers_for_work_stealing, &producer)) {

            /*
             * Use work stealing to get work from the producer that
             * belongs to another consumer.
             */
            concrete_actor->producer = producer;

            printf("DEBUG window %d asks new producer %d (work stealing)\n",
                    thorium_actor_name(actor),
                    producer);
            biosal_assembly_sliding_window_ask(actor, message);

        } else {

            printf("DEBUG %s/%d DONE\n",
                            thorium_actor_script_name(actor),
                    thorium_actor_name(actor));

            thorium_actor_send_empty(actor,
                                concrete_actor->producer_source,
                        ACTION_SET_PRODUCER_REPLY);
        }
    } else if (tag == ACTION_SET_CONSUMER_REPLY) {

        /* Set the producer for the new window
         */
        producer = concrete_actor->producer;

        thorium_actor_send_int(actor, concrete_actor->auto_scaling_clone,
                        ACTION_SET_PRODUCER, producer);

        concrete_actor->auto_scaling_in_progress = 0;
        concrete_actor->scaling_operations++;
        concrete_actor->auto_scaling_clone = THORIUM_ACTOR_NOBODY;

        printf("window %d completed auto-scaling # %d\n",
                        thorium_actor_name(actor),
                        concrete_actor->scaling_operations);

    } else if (tag == ACTION_SET_PRODUCER_REPLY
                    && source == concrete_actor->auto_scaling_clone) {

    } else if (tag == ACTION_ENABLE_AUTO_SCALING) {

        /*
         * auto-scaling is not implemented.
         */
#if 0
        printf("AUTO-SCALING window %d enables auto-scaling (ACTION_ENABLE_AUTO_SCALING) via actor %d\n",
                        name, source);

        thorium_actor_send_to_self_empty(actor, ACTION_ENABLE_AUTO_SCALING);
#endif
    }
}

void biosal_assembly_sliding_window_verify(struct thorium_actor *actor, struct thorium_message *message)
{
    struct biosal_assembly_sliding_window *concrete_actor;

    concrete_actor = (struct biosal_assembly_sliding_window *)thorium_actor_concrete_actor(actor);

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
                    ACTION_NOTIFY_REPLY, concrete_actor->kmers);
}

void biosal_assembly_sliding_window_ask(struct thorium_actor *self, struct thorium_message *message)
{
    struct biosal_assembly_sliding_window *concrete_actor;
    int producer;

    concrete_actor = (struct biosal_assembly_sliding_window *)thorium_actor_concrete_actor(self);

    producer = concrete_actor->producer;

    thorium_actor_send_int(self, producer, ACTION_SEQUENCE_STORE_ASK,
                    concrete_actor->kmer_length);

#ifdef BIOSAL_ASSEMBLY_SLIDING_WINDOW_DEBUG
    printf("DEBUG window %d asks producer %d\n",
                    thorium_actor_name(self),
                    producer);
#endif
}

void biosal_assembly_sliding_window_do_auto_scaling(struct thorium_actor *actor, struct thorium_message *message)
{
    struct biosal_assembly_sliding_window *concrete_actor;
    int name;
    int source;

    name = thorium_actor_name(actor);
    source = thorium_message_source(message);

    concrete_actor = (struct biosal_assembly_sliding_window *)thorium_actor_concrete_actor(actor);

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
    if (concrete_actor->producer == THORIUM_ACTOR_NOBODY) {
        return;
    }

    /* - spawn a kernel
     * - spawn an aggregator
     * - set the aggregator as the consumer of the kernel
     * - set the kmer stores as the consumers of the aggregator
     * - set a sequence store as the producer of the kernel (ACTION_SET_PRODUCER)
     */

    printf("AUTO-SCALING window %d receives auto-scale message (ACTION_DO_AUTO_SCALING) via actor %d\n",
                    name, source);

    concrete_actor->auto_scaling_in_progress = 1;

    thorium_actor_send_to_self_int(actor, ACTION_CLONE, name);
}

void biosal_assembly_sliding_window_pack_message(struct thorium_actor *actor, struct thorium_message *message)
{
    int *new_buffer;
    int new_count;
    struct thorium_message new_message;

    new_count = biosal_assembly_sliding_window_pack_size(actor);
    new_buffer = thorium_actor_allocate(actor, new_count);

    biosal_assembly_sliding_window_pack(actor, new_buffer);

    thorium_message_init(&new_message, ACTION_PACK_REPLY, new_count, new_buffer);

    thorium_actor_send_reply(actor, &new_message);

    thorium_message_destroy(&new_message);
}

void biosal_assembly_sliding_window_unpack_message(struct thorium_actor *actor, struct thorium_message *message)
{
    void *buffer;

    buffer = thorium_message_buffer(message);

    biosal_assembly_sliding_window_unpack(actor, buffer);

    thorium_actor_send_reply_empty(actor, ACTION_UNPACK_REPLY);
}

void biosal_assembly_sliding_window_clone_reply(struct thorium_actor *actor, struct thorium_message *message)
{
    struct biosal_assembly_sliding_window *concrete_actor;
    int name;
    int clone;
    int source;
    int consumer;
    /*int producer;*/
    int clone_index;

    source = thorium_message_source(message);
    concrete_actor = (struct biosal_assembly_sliding_window *)thorium_actor_concrete_actor(actor);
    name = thorium_actor_name(actor);
    thorium_message_unpack_int(message, 0, &clone);
    consumer = concrete_actor->consumer;
    /*producer = concrete_actor->producer);*/

    if (source == name) {
        printf("window %d cloned itself !!! clone name is %d\n",
                    name, clone);

        thorium_actor_send_int(actor, consumer, ACTION_CLONE, name);

        concrete_actor->auto_scaling_clone = clone;

        clone_index = clone;

        core_vector_push_back(&concrete_actor->kernels, &clone_index);

    } else if (source == consumer) {
        printf("window %d cloned aggregator %d, clone name is %d\n",
                        name, consumer, clone);

        thorium_actor_send_int(actor, concrete_actor->auto_scaling_clone,
                        ACTION_SET_CONSUMER, clone);

    }
}


int biosal_assembly_sliding_window_pack(struct thorium_actor *actor, void *buffer)
{
    return biosal_assembly_sliding_window_pack_unpack(actor, CORE_PACKER_OPERATION_PACK, buffer);
}

int biosal_assembly_sliding_window_unpack(struct thorium_actor *actor, void *buffer)
{
    return biosal_assembly_sliding_window_pack_unpack(actor, CORE_PACKER_OPERATION_UNPACK, buffer);
}

int biosal_assembly_sliding_window_pack_size(struct thorium_actor *actor)
{
    return biosal_assembly_sliding_window_pack_unpack(actor, CORE_PACKER_OPERATION_PACK_SIZE, NULL);
}

/*
 * Copy these:
 *
 * - kmer length
 * - consumer
 * - producer
 */
int biosal_assembly_sliding_window_pack_unpack(struct thorium_actor *actor, int operation, void *buffer)
{
    int bytes;
    struct core_packer packer;
    struct biosal_assembly_sliding_window *concrete_actor;
    int producer;
    int consumer;

    concrete_actor = (struct biosal_assembly_sliding_window *)thorium_actor_concrete_actor(actor);
    producer = THORIUM_ACTOR_NOBODY;
    consumer = THORIUM_ACTOR_NOBODY;

    if (operation != CORE_PACKER_OPERATION_UNPACK) {
        producer = concrete_actor->producer;
        consumer = concrete_actor->consumer;
    }

    bytes = 0;

    core_packer_init(&packer, operation, buffer);

    core_packer_process(&packer, &concrete_actor->kmer_length, sizeof(concrete_actor->kmer_length));
    core_packer_process(&packer, &producer, sizeof(producer));
    core_packer_process(&packer, &consumer, sizeof(consumer));

    if (operation == CORE_PACKER_OPERATION_UNPACK) {

        concrete_actor->producer = producer;
        concrete_actor->consumer = consumer;
    }

    bytes += core_packer_get_byte_count(&packer);
    core_packer_destroy(&packer);

    return bytes;
}

void biosal_assembly_sliding_window_notify(struct thorium_actor *actor, struct thorium_message *message)
{
    struct biosal_assembly_sliding_window *concrete_actor;
    struct core_vector_iterator iterator;
    int kernel;
    int source;

    concrete_actor = (struct biosal_assembly_sliding_window *)thorium_actor_concrete_actor(actor);

    source = thorium_message_source(message);
    concrete_actor->notified = 1;

    thorium_actor_send_to_self_empty(actor, ACTION_DISABLE_AUTO_SCALING);

    concrete_actor->notification_source = source;

    if (concrete_actor->scaling_operations > 0) {

        concrete_actor->sum_of_kmers = 0;
        concrete_actor->sum_of_kmers += concrete_actor->kmers;
        concrete_actor->notified_children = 0;

        core_vector_iterator_init(&iterator, &concrete_actor->kernels);

        while (core_vector_iterator_get_next_value(&iterator, &kernel)) {

            thorium_actor_send_empty(actor, kernel, ACTION_NOTIFY);
        }

        core_vector_iterator_destroy(&iterator);


    } else {
        biosal_assembly_sliding_window_verify(actor, message);
    }
}

void biosal_assembly_sliding_window_notify_reply(struct thorium_actor *actor, struct thorium_message *message)
{
    struct biosal_assembly_sliding_window *concrete_actor;
    uint64_t kmers;

    concrete_actor = (struct biosal_assembly_sliding_window *)thorium_actor_concrete_actor(actor);

    thorium_message_unpack_uint64_t(message, 0, &kmers);

    concrete_actor->sum_of_kmers += kmers;

    concrete_actor->notified_children++;

    if (concrete_actor->notified_children == core_vector_size(&concrete_actor->kernels)) {


        thorium_actor_send_uint64_t(actor,
                            concrete_actor->notification_source,
                    ACTION_NOTIFY_REPLY, concrete_actor->sum_of_kmers);
    }
}

void biosal_assembly_sliding_window_push_sequence_data_block(struct thorium_actor *actor, struct thorium_message *message)
{
    int source;
    struct biosal_dna_kmer kmer;
    int name;
    struct biosal_input_command payload;
    void *buffer;
    int entries;
    struct biosal_assembly_sliding_window *concrete_actor;
    int source_index;
    int consumer;
    int i;
    struct biosal_dna_sequence *sequence;
    char *sequence_data;
    struct core_vector *command_entries;
    int sequence_length;
    int new_count;
    void *new_buffer;
    struct thorium_message new_message;
    int j;
    int limit;
    char saved;
    struct core_timer timer;
    struct biosal_dna_kmer_block block;
    int to_reserve;
    int maximum_length;
    struct core_memory_pool *ephemeral_memory;
    int kmers_for_sequence;

    concrete_actor = (struct biosal_assembly_sliding_window *)thorium_actor_concrete_actor(actor);
    ephemeral_memory = thorium_actor_get_ephemeral_memory(actor);
    name = thorium_actor_name(actor);
    source = thorium_message_source(message);
    buffer = thorium_message_buffer(message);

    if (concrete_actor->kmer_length == THORIUM_ACTOR_NOBODY) {
        printf("Error no kmer length set\n");
        return;
    }

    if (concrete_actor->consumer == THORIUM_ACTOR_NOBODY) {
        printf("Error no consumer set\n");
        return;
    }

    if (concrete_actor->producer_source == THORIUM_ACTOR_NOBODY) {
        printf("Error, no producer_source set\n");
        return;
    }

    core_timer_init(&timer);
    core_timer_start(&timer);

    consumer = concrete_actor->consumer;
    source_index = source;

    biosal_input_command_init_empty(&payload);
    biosal_input_command_unpack(&payload, buffer, thorium_actor_get_ephemeral_memory(actor),
                    &concrete_actor->codec);

    command_entries = biosal_input_command_entries(&payload);

    entries = core_vector_size(command_entries);

    if (entries == 0) {
        printf("Error: received empty payload...\n");
    }

    to_reserve = 0;
    maximum_length = 0;

    for (i = 0; i < entries; i++) {

        sequence = (struct biosal_dna_sequence *)core_vector_at(command_entries, i);

        sequence_length = biosal_dna_sequence_length(sequence);

        if (sequence_length > maximum_length) {
            maximum_length = sequence_length;
        }

        to_reserve += (sequence_length - concrete_actor->kmer_length + 1);
    }

    biosal_dna_kmer_block_init(&block, concrete_actor->kmer_length, source_index, to_reserve,
                    ephemeral_memory);

    sequence_data = core_memory_pool_allocate(ephemeral_memory, maximum_length + 1);

    /* extract kmers
     */
    for (i = 0; i < entries; i++) {

        sequence = (struct biosal_dna_sequence *)core_vector_at(command_entries, i);

        biosal_dna_sequence_get_sequence(sequence, sequence_data,
                        &concrete_actor->codec);

        sequence_length = biosal_dna_sequence_length(sequence);
        limit = sequence_length - concrete_actor->kmer_length + 1;

        kmers_for_sequence = 0;

        for (j = 0; j < limit; j++) {
            saved = sequence_data[j + concrete_actor->kmer_length];
            sequence_data[j + concrete_actor->kmer_length] = '\0';

            biosal_dna_kmer_init(&kmer, sequence_data + j,
                            &concrete_actor->codec, thorium_actor_get_ephemeral_memory(actor));

#ifdef BIOSAL_WINDOW_DEBUG_LEVEL_2
            printf("KERNEL kmer %d,%d %s\n", i, j, sequence_data + j);
#endif

            /*
             * add kmer in block
             */
            biosal_dna_kmer_block_add_kmer(&block, &kmer, thorium_actor_get_ephemeral_memory(actor),
                            &concrete_actor->codec);

            biosal_dna_kmer_destroy(&kmer, thorium_actor_get_ephemeral_memory(actor));

            sequence_data[j + concrete_actor->kmer_length] = saved;

            ++kmers_for_sequence;
        }

#ifdef BIOSAL_PRIVATE_DEBUG_EMIT
        printf("DEBUG EMIT KMERS INPUT: %d nucleotides, k: %d output %d kmers\n",
                        sequence_length, concrete_actor->kmer_length,
                        kmers_for_sequence);
#endif

        concrete_actor->kmers += kmers_for_sequence;
    }

    core_memory_pool_free(ephemeral_memory, sequence_data);
    sequence_data = NULL;

#ifdef BIOSAL_WINDOW_DEBUG
    BIOSAL_DEBUG_MARKER("after generating kmers\n");
#endif

    concrete_actor->actual += entries;
    concrete_actor->blocks++;

    biosal_input_command_destroy(&payload, thorium_actor_get_ephemeral_memory(actor));

    new_count = biosal_dna_kmer_block_pack_size(&block,
                    &concrete_actor->codec);
    new_buffer = thorium_actor_allocate(actor, new_count);
    biosal_dna_kmer_block_pack(&block, new_buffer,
                        &concrete_actor->codec);

#ifdef BIOSAL_WINDOW_DEBUG
    printf("name %d destination %d PACK with %d bytes\n", name,
                       consumer, new_count);
#endif


    thorium_message_init(&new_message, ACTION_AGGREGATE_KERNEL_OUTPUT,
                    new_count, new_buffer);

    /*
    thorium_message_init(&new_message, ACTION_AGGREGATE_KERNEL_OUTPUT,
                    sizeof(source_index), &source_index);
                    */

    thorium_actor_send(actor, consumer, &new_message);

    ++concrete_actor->flushed_payloads;

    thorium_actor_send_empty(actor,
                    source_index,
                    ACTION_PUSH_SEQUENCE_DATA_BLOCK_REPLY);

    if (concrete_actor->actual == concrete_actor->expected
                    || concrete_actor->actual >= concrete_actor->last + 300000
                    || concrete_actor->last == 0) {

        printf("sliding window %d processed %" PRIu64 " entries (%d blocks) so far\n",
                        name, concrete_actor->actual,
                        concrete_actor->blocks);

        concrete_actor->last = concrete_actor->actual;
    }

    core_timer_stop(&timer);

#ifdef BIOSAL_WINDOW_DEBUG

        core_timer_print(&timer);
#endif

    core_timer_destroy(&timer);

    biosal_dna_kmer_block_destroy(&block, thorium_actor_get_ephemeral_memory(actor));

#ifdef BIOSAL_WINDOW_DEBUG
    BIOSAL_DEBUG_MARKER("leaving call.\n");
#endif

    biosal_assembly_sliding_window_verify(actor, message);
}

void biosal_assembly_sliding_window_set_producers_for_work_stealing(struct thorium_actor *self, struct thorium_message *message)
{
    struct biosal_assembly_sliding_window *concrete_self;
    struct core_memory_pool *ephemeral_memory;
    void *buffer;
    struct core_vector producers;
    int i;
    int size;
    int producer;

    buffer = thorium_message_buffer(message);
    concrete_self = (struct biosal_assembly_sliding_window *)thorium_actor_concrete_actor(self);
    ephemeral_memory = thorium_actor_get_ephemeral_memory(self);

    core_vector_init(&producers, sizeof(int));
    core_vector_set_memory_pool(&producers, ephemeral_memory);
    core_vector_unpack(&producers, buffer);

    i = 0;
    size = core_vector_size(&producers);

    while (i < size) {
        producer = core_vector_at_as_int(&producers, i);

        core_fast_queue_enqueue(&concrete_self->producers_for_work_stealing, &producer);

        ++i;
    }

    core_vector_destroy(&producers);

    thorium_actor_send_reply_empty(self, ACTION_SET_PRODUCERS_FOR_WORK_STEALING_REPLY);
}
