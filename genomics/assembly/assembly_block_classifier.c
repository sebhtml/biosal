
#include "assembly_block_classifier.h"

#include <genomics/data/dna_kmer_block.h>
#include <genomics/data/dna_kmer_frequency_block.h>
#include <genomics/data/dna_kmer.h>

/*
 * For the message tags
 */
#include <genomics/kernels/aggregator.h>
#include <genomics/kernels/dna_kmer_counter_kernel.h>

#include <genomics/storage/sequence_store.h>
#include <genomics/storage/kmer_store.h>

#include <engine/thorium/modules/message_helper.h>
#include <core/helpers/vector_helper.h>

#include <core/system/packer.h>

#include <core/structures/vector_iterator.h>

#include <core/system/debugger.h>
#include <core/system/memory.h>

#include <biosal.h>

#include <stdio.h>
#include <inttypes.h>

#define MEMORY_POOL_NAME_VERTEX_CLASSIFIER 0xa156d064

/*
 * Disable memory tracking for increased
 * performance
 */
#define BIOSAL_ASSEMBLY_BLOCK_CLASSIFIER_DISABLE_TRACKING

/*
#define BIOSAL_ASSEMBLY_BLOCK_CLASSIFIER_DEBUG_FLUSHING
*/

void biosal_assembly_block_classifier_init(struct thorium_actor *actor);
void biosal_assembly_block_classifier_destroy(struct thorium_actor *actor);
void biosal_assembly_block_classifier_receive(struct thorium_actor *actor, struct thorium_message *message);

void biosal_assembly_block_classifier_flush(struct thorium_actor *self, int customer_index, struct core_vector *buffers,
                int force);
void biosal_assembly_block_classifier_flush_all(struct thorium_actor *self);

void biosal_assembly_block_classifier_verify(struct thorium_actor *self, struct thorium_message *message);
void biosal_assembly_block_classifier_aggregate_kernel_output(struct thorium_actor *self, struct thorium_message *message);

void biosal_assembly_block_classifier_unpack_message(struct thorium_actor *actor, struct thorium_message *message);
void biosal_assembly_block_classifier_pack_message(struct thorium_actor *actor, struct thorium_message *message);
int biosal_assembly_block_classifier_set_consumers(struct thorium_actor *actor, void *buffer);

int biosal_assembly_block_classifier_pack_unpack(struct thorium_actor *actor, int operation, void *buffer);
int biosal_assembly_block_classifier_pack(struct thorium_actor *actor, void *buffer);
int biosal_assembly_block_classifier_unpack(struct thorium_actor *actor, void *buffer);
int biosal_assembly_block_classifier_pack_size(struct thorium_actor *actor);

struct thorium_script biosal_assembly_block_classifier_script = {
    .identifier = SCRIPT_ASSEMBLY_BLOCK_CLASSIFIER,
    .name = "biosal_assembly_block_classifier",
    .description = "Classifier for blocks of kmers.",
    .version = "",
    .author = "Sebastien Boisvert",
    .size = sizeof(struct biosal_assembly_block_classifier),
    .init = biosal_assembly_block_classifier_init,
    .destroy = biosal_assembly_block_classifier_destroy,
    .receive = biosal_assembly_block_classifier_receive
};

void biosal_assembly_block_classifier_init(struct thorium_actor *self)
{
    struct biosal_assembly_block_classifier *concrete_actor;

    concrete_actor = (struct biosal_assembly_block_classifier *)thorium_actor_concrete_actor(self);

    core_memory_pool_init(&concrete_actor->persistent_pool, 16 * 1048576, MEMORY_POOL_NAME_VERTEX_CLASSIFIER);

    concrete_actor->received = 0;
    concrete_actor->last = 0;

    core_vector_init(&concrete_actor->consumers, sizeof(int));

    concrete_actor->flushed = 0;

    biosal_dna_codec_init(&concrete_actor->codec);

    if (biosal_dna_codec_must_use_two_bit_encoding(&concrete_actor->codec,
                            thorium_actor_get_node_count(self))) {
        biosal_dna_codec_enable_two_bit_encoding(&concrete_actor->codec);
    }

    core_queue_init(&concrete_actor->stalled_producers, sizeof(int));
    core_vector_init(&concrete_actor->active_messages, sizeof(int));

    concrete_actor->active_requests = 0;
    concrete_actor->forced = 0;

    /* Enable cloning stuff
     */
    thorium_actor_send_to_self_empty(self, ACTION_PACK_ENABLE);

    thorium_actor_add_action(self, ACTION_PACK,
                    biosal_assembly_block_classifier_pack_message);
    thorium_actor_add_action(self, ACTION_UNPACK,
                    biosal_assembly_block_classifier_unpack_message);
    thorium_actor_add_action(self, ACTION_AGGREGATE_KERNEL_OUTPUT,
                    biosal_assembly_block_classifier_aggregate_kernel_output);

    printf("assembly_block_classifier %d is online\n", thorium_actor_name(self));

    concrete_actor->consumer_count_with_maximum = 0;

    core_vector_init(&concrete_actor->buffers, 0);

    concrete_actor->customer_block_size = 42;
}

void biosal_assembly_block_classifier_destroy(struct thorium_actor *self)
{
    struct biosal_assembly_block_classifier *concrete_actor;
    struct core_vector_iterator iterator;
    struct biosal_dna_kmer_frequency_block *output_block;

    concrete_actor = (struct biosal_assembly_block_classifier *)thorium_actor_concrete_actor(self);

    core_vector_destroy(&concrete_actor->active_messages);
    concrete_actor->received = 0;
    concrete_actor->last = 0;
    biosal_dna_codec_destroy(&concrete_actor->codec);

    core_queue_destroy(&concrete_actor->stalled_producers);

    core_vector_destroy(&concrete_actor->consumers);

    core_vector_iterator_init(&iterator, &concrete_actor->buffers);

    while (core_vector_iterator_has_next(&iterator)) {

        core_vector_iterator_next(&iterator, (void **)&output_block);

        /*
         * Destroy block
         */
        biosal_dna_kmer_frequency_block_destroy(output_block, &concrete_actor->persistent_pool);
    }

    core_vector_iterator_destroy(&iterator);

    core_vector_destroy(&concrete_actor->buffers);

    core_memory_pool_examine(&concrete_actor->persistent_pool);
    core_memory_pool_destroy(&concrete_actor->persistent_pool);

    printf("ISSUE-819 biosal_assembly_block_classifier dies\n");
}

void biosal_assembly_block_classifier_receive(struct thorium_actor *self, struct thorium_message *message)
{
    int tag;
    struct biosal_assembly_block_classifier *concrete_actor;
    void *buffer;
    int source;
    int consumer_index_index;
    int *bucket;

    if (thorium_actor_take_action(self, message)) {
        return;
    }

    concrete_actor = (struct biosal_assembly_block_classifier *)thorium_actor_concrete_actor(self);
    buffer = thorium_message_buffer(message);
    tag = thorium_message_action(message);
    source = thorium_message_source(message);

    if (tag == ACTION_ASK_TO_STOP) {

        thorium_actor_ask_to_stop(self, message);

    } else if (tag == ACTION_SET_KMER_LENGTH) {

        thorium_message_unpack_int(message, 0, &concrete_actor->kmer_length);

        thorium_actor_send_reply_empty(self, ACTION_SET_KMER_LENGTH_REPLY);

    } else if (tag == ACTION_SET_CONSUMERS) {

        biosal_assembly_block_classifier_set_consumers(self, buffer);

        thorium_actor_send_reply_empty(self, ACTION_SET_CONSUMERS_REPLY);

    } else if (tag == ACTION_AGGREGATOR_FLUSH) {

        concrete_actor->forced = 1;

        /*
        printf("block classifier, ACTION_AGGREGATOR_FLUSH flush all\n");
        */

        biosal_assembly_block_classifier_flush_all(self);

        thorium_actor_send_reply_empty(self, ACTION_AGGREGATOR_FLUSH_REPLY);

    } else if (tag == ACTION_PUSH_KMER_BLOCK_REPLY) {

#ifdef BIOSAL_ASSEMBLY_BLOCK_CLASSIFIER_DEBUG
        printf("BEFORE ACTION_PUSH_KMER_BLOCK_REPLY %d\n", concrete_actor->active_messages);
#endif

        consumer_index_index = core_vector_index_of(&concrete_actor->consumers, &source);

        bucket = (int *)core_vector_at(&concrete_actor->active_messages, consumer_index_index);

        if (*bucket == concrete_actor->maximum_active_messages) {

            /*
             * The count was maximum_active_messages
             * before removing 1 from it.
             */
            --concrete_actor->consumer_count_with_maximum;
        }

        (*bucket)--;
        --concrete_actor->active_requests;

        if (!concrete_actor->forced) {
            biosal_assembly_block_classifier_verify(self, message);
        }
    }
}

void biosal_assembly_block_classifier_flush(struct thorium_actor *self, int customer_index, struct core_vector *buffers,
                int force)
{
    struct biosal_dna_kmer_frequency_block *customer_block_pointer;
    struct biosal_assembly_block_classifier *concrete_actor;
    int count;
    void *buffer;
    struct thorium_message message;
    int customer;
    /*
    struct core_memory_pool *ephemeral_memory;
    */
    int threshold;
    int *bucket;

    /*
     * Only flush when required.
     */
    threshold = thorium_actor_suggested_buffer_size(self);

    concrete_actor = (struct biosal_assembly_block_classifier *)thorium_actor_concrete_actor(self);

    /*
    ephemeral_memory = thorium_actor_get_ephemeral_memory(self);
    */
    customer = core_vector_at_as_int(&concrete_actor->consumers, customer_index);
    customer_block_pointer = (struct biosal_dna_kmer_frequency_block *)core_vector_at(buffers, customer_index);

    CORE_DEBUGGER_ASSERT(customer_block_pointer != NULL);

    /*
     * Nothing to flush.
     */
    if (biosal_dna_kmer_frequency_block_empty(customer_block_pointer)) {
        return;
    }

    count = biosal_dna_kmer_frequency_block_pack_size(customer_block_pointer,
                    &concrete_actor->codec);

    /*
    printf("%d flushes %d bytes\n",
                    thorium_actor_name(self), count);
                    */

    /*
     * Don't flush if the force parameter is not set and there are not enough
     * bytes.
     */
    if (!force && count < threshold) {
        return;
    }

#ifdef BIOSAL_ASSEMBLY_BLOCK_CLASSIFIER_DEBUG_FLUSHING
    printf("DEBUG biosal_assembly_block_classifier_flush actual %d threshold %d\n", count,
                    threshold);
#endif

    buffer = thorium_actor_allocate(self, count);
    biosal_dna_kmer_frequency_block_pack(customer_block_pointer, buffer,
                    &concrete_actor->codec);

    thorium_message_init(&message, ACTION_PUSH_KMER_BLOCK, count, buffer);
    thorium_actor_send(self, customer, &message);

    bucket = core_vector_at(&concrete_actor->active_messages, customer_index);
    (*bucket)++;
    ++concrete_actor->active_requests;

    /*
     * Increment event counter
     */
    if (*bucket == concrete_actor->maximum_active_messages) {
        ++concrete_actor->consumer_count_with_maximum;
    }

    thorium_message_destroy(&message);

    buffer = NULL;

    if (concrete_actor->flushed % 50000 == 0) {
        printf("assembly_block_classifier %d flushed %d blocks so far\n",
                        thorium_actor_name(self), concrete_actor->flushed);
    }

    /*
     * Reset the buffer
     */
    biosal_dna_kmer_frequency_block_destroy(customer_block_pointer, &concrete_actor->persistent_pool);

    biosal_dna_kmer_frequency_block_init(customer_block_pointer, concrete_actor->kmer_length,
                    &concrete_actor->persistent_pool, &concrete_actor->codec,
                        concrete_actor->customer_block_size);

    concrete_actor->flushed++;
}

void biosal_assembly_block_classifier_verify(struct thorium_actor *self, struct thorium_message *message)
{
    struct biosal_assembly_block_classifier *concrete_actor;
    int producer_index;
    int producer;
#if 0
    int consumer_index;
#endif

    concrete_actor = (struct biosal_assembly_block_classifier *)thorium_actor_concrete_actor(self);

    /* Only continue if there are not too many
     * active messages.
     */

#if 0
    size = core_vector_size(&concrete_actor->active_messages);

    for (consumer_index = 0; consumer_index < size; consumer_index++) {
        bucket = (int *)core_vector_at(&concrete_actor->active_messages, consumer_index);

        /*
         * Not ready yet because the maximum is still
         * being used.
         */
        if (*bucket >= concrete_actor->maximum_active_messages) {
            return;
        }
    }
#endif

    if (concrete_actor->consumer_count_with_maximum > 0) {
        return;
    }

    /*
     * The code here is to make sure that there is enough memory.
     * If there is 0 active requests, then a reply must be sent
     * anyway even if the system is out of memory.
     */
    if (concrete_actor->active_requests > 0
                    && !core_memory_has_enough_bytes()) {
        return;
    }

    while (core_queue_dequeue(&concrete_actor->stalled_producers, &producer_index)) {
        /* tell the producer to continue...
         */
        producer = producer_index;
        thorium_actor_send_empty(self, producer, ACTION_AGGREGATE_KERNEL_OUTPUT_REPLY);
    }
}

void biosal_assembly_block_classifier_aggregate_kernel_output(struct thorium_actor *self, struct thorium_message *message)
{
    int i;
    struct biosal_assembly_block_classifier *concrete_actor;
    int producer_index;
    int customer_count;
    struct biosal_dna_kmer_frequency_block *customer_block_pointer;
    int entries;
    struct biosal_dna_kmer_block input_block;
    struct biosal_dna_kmer_frequency_block *output_block;
    struct core_vector *kmers;
    struct core_memory_pool *ephemeral_memory;
    struct biosal_dna_kmer *kmer;
    int source;
    void *buffer;
    int customer_index;
    struct core_vector_iterator iterator;

    concrete_actor = (struct biosal_assembly_block_classifier *)thorium_actor_concrete_actor(self);

    if (core_vector_size(&concrete_actor->consumers) == 0) {
        printf("Error: classifier %d has no configured buffers\n",
                        thorium_actor_name(self));
        return;
    }

    ephemeral_memory = thorium_actor_get_ephemeral_memory(self);
    source = thorium_message_source(message);
    buffer = thorium_message_buffer(message);

    /* enqueue the producer
     */
    producer_index = source;
    core_queue_enqueue(&concrete_actor->stalled_producers, &producer_index);


#ifdef BIOSAL_ASSEMBLY_BLOCK_CLASSIFIER_DEBUG
    BIOSAL_DEBUG_MARKER("assembly_block_classifier receives");
    printf("name %d source %d UNPACK ON %d bytes\n",
                        thorium_actor_name(self), source, thorium_message_count(message));
#endif

    concrete_actor->received++;

    CORE_DEBUGGER_LEAK_DETECTION_BEGIN(ephemeral_memory, input);

    biosal_dna_kmer_block_init_empty(&input_block);
    biosal_dna_kmer_block_unpack(&input_block, buffer, ephemeral_memory,
                        &concrete_actor->codec);

#ifdef BIOSAL_ASSEMBLY_BLOCK_CLASSIFIER_DEBUG
        BIOSAL_DEBUG_MARKER("assembly_block_classifier before loop");
#endif

    /*
     * classify the kmers according to their ownership
     */

    kmers = biosal_dna_kmer_block_kmers(&input_block);
    entries = core_vector_size(kmers);

    customer_count = core_vector_size(&concrete_actor->consumers);

    /*
     * Actually populate buffers.
     */
    for (i = 0; i < entries; i++) {

        CORE_DEBUGGER_LEAK_DETECTION_BEGIN(ephemeral_memory, iteration);

        kmer = (struct biosal_dna_kmer *)core_vector_at(kmers, i);

        /*
        biosal_dna_kmer_print(kmer);
        biosal_dna_kmer_length(kmer);
        */

        customer_index = biosal_dna_kmer_store_index(kmer, customer_count, concrete_actor->kmer_length,
                        &concrete_actor->codec, ephemeral_memory);

        customer_block_pointer = (struct biosal_dna_kmer_frequency_block *)core_vector_at(&concrete_actor->buffers,
                        customer_index);

#ifdef BIOSAL_ASSEMBLY_BLOCK_CLASSIFIER_DEBUG
        printf("DEBUG customer_index %d block pointer %p\n",
                        customer_index, (void *)customer_block_pointer);

        BIOSAL_DEBUG_MARKER("assembly_block_classifier before add");
#endif

        /*
         * add kmer to buffer and try to flush
         */

        CORE_DEBUGGER_LEAK_DETECTION_BEGIN(ephemeral_memory, add);

        /* classify the kmer and put it in the good buffer.
         */
        biosal_dna_kmer_frequency_block_add_kmer(customer_block_pointer, kmer,
                        &concrete_actor->persistent_pool,
                        &concrete_actor->codec);

        CORE_DEBUGGER_LEAK_DETECTION_END(ephemeral_memory, add);

        /*
         * The Flush() action below is only required when using
         * biosal_dna_kmer_block.
         * biosal_dna_kmer_frequency_block does not require this
         * flush operation.
         */
#if 0
        /* Flush if necessary to avoid having very big buffers
         */
        if (0 && i % 32 == 0) {
            biosal_assembly_block_classifier_flush(self, customer_index, &buffers, 0);
        }
#endif

#ifdef BIOSAL_ASSEMBLY_BLOCK_CLASSIFIER_DEBUG
        BIOSAL_DEBUG_MARKER("assembly_block_classifier before flush");
#endif

        CORE_DEBUGGER_LEAK_DETECTION_END(ephemeral_memory, iteration);
    }

#ifdef BIOSAL_ASSEMBLY_BLOCK_CLASSIFIER_DEBUG
    BIOSAL_DEBUG_MARKER("assembly_block_classifier after loop");
#endif

    if (concrete_actor->last == 0
                    || concrete_actor->received >= concrete_actor->last + 10000) {

        printf("assembly_block_classifier/%d received %" PRIu64 " kernel outputs so far\n",
                        thorium_actor_name(self),
                        concrete_actor->received);

        concrete_actor->last = concrete_actor->received;
    }

    /* destroy the local copy of the block
     */
    biosal_dna_kmer_block_destroy(&input_block, ephemeral_memory);

    CORE_DEBUGGER_LEAK_DETECTION_END(ephemeral_memory, input);

#ifdef BIOSAL_ASSEMBLY_BLOCK_CLASSIFIER_DEBUG
        BIOSAL_DEBUG_MARKER("assembly_block_classifier marker EXIT");
#endif

    /*
     * Flush blocks if they are big enough.
     */

    core_vector_iterator_init(&iterator, &concrete_actor->buffers);

    i = 0;
    while (core_vector_iterator_has_next(&iterator)) {

        core_vector_iterator_next(&iterator, (void **)&output_block);

        customer_index = i;

#ifdef BIOSAL_ASSEMBLY_BLOCK_CLASSIFIER_DEBUG
        printf("assembly_block_classifier flushing %d\n", customer_index);
#endif

        biosal_assembly_block_classifier_flush(self, customer_index, &concrete_actor->buffers, 0);

#if 0
        /*
         * Destroy block
         */
        biosal_dna_kmer_frequency_block_destroy(output_block, ephemeral_memory);
#endif

        i++;
    }

    core_vector_iterator_destroy(&iterator);

    biosal_assembly_block_classifier_verify(self, message);
}

void biosal_assembly_block_classifier_pack_message(struct thorium_actor *actor, struct thorium_message *message)
{
    void *new_buffer;
    int new_count;
    struct thorium_message new_message;

    new_count = biosal_assembly_block_classifier_pack_size(actor);
    new_buffer = thorium_actor_allocate(actor, new_count);

    biosal_assembly_block_classifier_pack(actor, new_buffer);

    thorium_message_init(&new_message, ACTION_PACK_REPLY, new_count, new_buffer);
    thorium_actor_send_reply(actor, &new_message);
    thorium_message_destroy(&new_message);

    new_buffer = NULL;
}

void biosal_assembly_block_classifier_unpack_message(struct thorium_actor *actor, struct thorium_message *message)
{
    void *buffer;

    buffer = thorium_message_buffer(message);

    biosal_assembly_block_classifier_unpack(actor, buffer);

    thorium_actor_send_reply_empty(actor, ACTION_UNPACK_REPLY);
}

int biosal_assembly_block_classifier_set_consumers(struct thorium_actor *actor, void *buffer)
{
    struct biosal_assembly_block_classifier *concrete_actor;
    int bytes;
    int size;
    int i;
    int zero;
    struct biosal_dna_kmer_frequency_block *customer_block_pointer;

    concrete_actor = (struct biosal_assembly_block_classifier *)thorium_actor_concrete_actor(actor);

    /*
     * receive customer list
     */
    core_vector_init(&concrete_actor->consumers, 0);
    bytes = core_vector_unpack(&concrete_actor->consumers, buffer);

    size = core_vector_size(&concrete_actor->consumers);

    core_vector_resize(&concrete_actor->active_messages, size);

    zero = 0;

    for (i = 0; i < size; i++) {
        core_vector_set(&concrete_actor->active_messages, i, &zero);
    }

    /*
     * The maximum number of active messages for any consumer is
     * set here.
     */
    concrete_actor->maximum_active_messages = thorium_actor_active_message_limit(actor);

    printf("DEBUG45 classifier %d preparing %d buffers, kmer_length %d, ACTIVE_MESSAGE_LIMIT %d\n",
                    thorium_actor_name(actor),
                        (int)core_vector_size(&concrete_actor->consumers),
                        concrete_actor->kmer_length,
                        concrete_actor->maximum_active_messages);

#ifdef BIOSAL_ASSEMBLY_BLOCK_CLASSIFIER_DEBUG
    printf("DEBUG classifier configured %d consumers\n",
                        (int)core_vector_size(&concrete_actor->consumers));
#endif

    /*
     * Create bins.
     */
    core_vector_init(&concrete_actor->buffers, sizeof(struct biosal_dna_kmer_frequency_block));
    core_vector_set_memory_pool(&concrete_actor->buffers, &concrete_actor->persistent_pool);

    core_vector_resize(&concrete_actor->buffers,
                    core_vector_size(&concrete_actor->consumers));

    /*
     * Reserve entries
     */

    for (i = 0; i < core_vector_size(&concrete_actor->consumers); i++) {

        customer_block_pointer = (struct biosal_dna_kmer_frequency_block *)core_vector_at(&concrete_actor->buffers,
                        i);
        biosal_dna_kmer_frequency_block_init(customer_block_pointer, concrete_actor->kmer_length,
                        &concrete_actor->persistent_pool, &concrete_actor->codec,
                        concrete_actor->customer_block_size);
    }

    return bytes;
}

int biosal_assembly_block_classifier_pack_unpack(struct thorium_actor *actor, int operation, void *buffer)
{
    struct core_packer packer;
    int bytes;
    struct biosal_assembly_block_classifier *concrete_actor;

    concrete_actor = (struct biosal_assembly_block_classifier *)thorium_actor_concrete_actor(actor);

    bytes = 0;

    core_packer_init(&packer, operation, buffer);

    /*
     * Pack the kmer length
     */
    core_packer_process(&packer, &concrete_actor->kmer_length, sizeof(concrete_actor->kmer_length));

    bytes += core_packer_get_byte_count(&packer);

    /*
    if (operation == CORE_PACKER_OPERATION_UNPACK) {
        printf("assembly_block_classifier %d unpacked kmer length %d\n",
                        thorium_actor_name(actor),
                        concrete_actor->kmer_length);
    }
*/
    /* Pack the consumers
     */

    if (operation == CORE_PACKER_OPERATION_UNPACK) {
        biosal_assembly_block_classifier_set_consumers(actor,
                        ((char *)buffer) + bytes);

    } else {

        bytes += core_vector_pack_unpack(&concrete_actor->consumers,
                        (char *)buffer + bytes,
                        operation);
    }

    core_packer_destroy(&packer);

    return bytes;
}

int biosal_assembly_block_classifier_pack(struct thorium_actor *actor, void *buffer)
{
    return biosal_assembly_block_classifier_pack_unpack(actor, CORE_PACKER_OPERATION_PACK, buffer);
}

int biosal_assembly_block_classifier_unpack(struct thorium_actor *actor, void *buffer)
{
    return biosal_assembly_block_classifier_pack_unpack(actor, CORE_PACKER_OPERATION_UNPACK, buffer);
}

int biosal_assembly_block_classifier_pack_size(struct thorium_actor *actor)
{
    return biosal_assembly_block_classifier_pack_unpack(actor, CORE_PACKER_OPERATION_PACK_SIZE, NULL);
}

void biosal_assembly_block_classifier_flush_all(struct thorium_actor *self)
{
    int i;
    int customer_index;
    struct core_vector_iterator iterator;
    struct biosal_dna_kmer_frequency_block *output_block;
    struct biosal_assembly_block_classifier *concrete_actor;

    concrete_actor = (struct biosal_assembly_block_classifier *)thorium_actor_concrete_actor(self);

    /*
     * Flush blocks
     */

    core_vector_iterator_init(&iterator, &concrete_actor->buffers);

    i = 0;
    while (core_vector_iterator_has_next(&iterator)) {

        core_vector_iterator_next(&iterator, (void **)&output_block);

        customer_index = i;

        biosal_assembly_block_classifier_flush(self, customer_index, &concrete_actor->buffers, 1);

        i++;
    }

    core_vector_iterator_destroy(&iterator);
}
