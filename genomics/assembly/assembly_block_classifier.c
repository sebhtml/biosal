
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

#include <core/helpers/message_helper.h>
#include <core/helpers/vector_helper.h>

#include <core/system/packer.h>

#include <core/structures/vector_iterator.h>

#include <core/system/debugger.h>
#include <core/system/memory.h>

#include <stdio.h>
#include <inttypes.h>

/*
 * Disable memory tracking for increased
 * performance
 */
#define BSAL_ASSEMBLY_BLOCK_CLASSIFIER_DISABLE_TRACKING

/*
#define BSAL_ASSEMBLY_BLOCK_CLASSIFIER_DEBUG_FLUSHING
*/

struct thorium_script bsal_assembly_block_classifier_script = {
    .identifier = SCRIPT_ASSEMBLY_BLOCK_CLASSIFIER,
    .name = "bsal_assembly_block_classifier",
    .description = "Classifier for blocks of kmers.",
    .version = "",
    .author = "Sebastien Boisvert",
    .size = sizeof(struct bsal_assembly_block_classifier),
    .init = bsal_assembly_block_classifier_init,
    .destroy = bsal_assembly_block_classifier_destroy,
    .receive = bsal_assembly_block_classifier_receive
};

void bsal_assembly_block_classifier_init(struct thorium_actor *self)
{
    struct bsal_assembly_block_classifier *concrete_actor;

    concrete_actor = (struct bsal_assembly_block_classifier *)thorium_actor_concrete_actor(self);
    concrete_actor->received = 0;
    concrete_actor->last = 0;

    bsal_vector_init(&concrete_actor->consumers, sizeof(int));

    concrete_actor->flushed = 0;

    bsal_dna_codec_init(&concrete_actor->codec);

    if (bsal_dna_codec_must_use_two_bit_encoding(&concrete_actor->codec,
                            thorium_actor_get_node_count(self))) {
        bsal_dna_codec_enable_two_bit_encoding(&concrete_actor->codec);
    }

    bsal_fast_queue_init(&concrete_actor->stalled_producers, sizeof(int));
    bsal_vector_init(&concrete_actor->active_messages, sizeof(int));

    concrete_actor->active_requests = 0;
    concrete_actor->forced = 0;

    /* Enable cloning stuff
     */
    thorium_actor_send_to_self_empty(self, ACTION_PACK_ENABLE);

    thorium_actor_add_action(self, ACTION_PACK,
                    bsal_assembly_block_classifier_pack_message);
    thorium_actor_add_action(self, ACTION_UNPACK,
                    bsal_assembly_block_classifier_unpack_message);
    thorium_actor_add_action(self, ACTION_AGGREGATE_KERNEL_OUTPUT,
                    bsal_assembly_block_classifier_aggregate_kernel_output);

    printf("assembly_block_classifier %d is online\n", thorium_actor_name(self));

    concrete_actor->consumer_count_above_threshold = 0;
}

void bsal_assembly_block_classifier_destroy(struct thorium_actor *self)
{
    struct bsal_assembly_block_classifier *concrete_actor;

    concrete_actor = (struct bsal_assembly_block_classifier *)thorium_actor_concrete_actor(self);

    bsal_vector_destroy(&concrete_actor->active_messages);
    concrete_actor->received = 0;
    concrete_actor->last = 0;
    bsal_dna_codec_destroy(&concrete_actor->codec);

    bsal_fast_queue_destroy(&concrete_actor->stalled_producers);

    bsal_vector_destroy(&concrete_actor->consumers);
}

void bsal_assembly_block_classifier_receive(struct thorium_actor *self, struct thorium_message *message)
{
    int tag;
    struct bsal_assembly_block_classifier *concrete_actor;
    void *buffer;
    int source;
    int consumer_index_index;
    int *bucket;

    if (thorium_actor_take_action(self, message)) {
        return;
    }

    concrete_actor = (struct bsal_assembly_block_classifier *)thorium_actor_concrete_actor(self);
    buffer = thorium_message_buffer(message);
    tag = thorium_message_tag(message);
    source = thorium_message_source(message);

    if (tag == ACTION_ASK_TO_STOP) {

        thorium_actor_ask_to_stop(self, message);

    } else if (tag == ACTION_SET_KMER_LENGTH) {

        thorium_message_unpack_int(message, 0, &concrete_actor->kmer_length);

        thorium_actor_send_reply_empty(self, ACTION_SET_KMER_LENGTH_REPLY);

    } else if (tag == ACTION_SET_CONSUMERS) {

        bsal_assembly_block_classifier_set_consumers(self, buffer);

        thorium_actor_send_reply_empty(self, ACTION_SET_CONSUMERS_REPLY);

    } else if (tag == ACTION_AGGREGATOR_FLUSH) {


        concrete_actor->forced = 1;

        thorium_actor_send_reply_empty(self, ACTION_AGGREGATOR_FLUSH_REPLY);

    } else if (tag == ACTION_PUSH_KMER_BLOCK_REPLY) {

#ifdef BSAL_ASSEMBLY_BLOCK_CLASSIFIER_DEBUG
        printf("BEFORE ACTION_PUSH_KMER_BLOCK_REPLY %d\n", concrete_actor->active_messages);
#endif

        consumer_index_index = bsal_vector_index_of(&concrete_actor->consumers, &source);

        bucket = (int *)bsal_vector_at(&concrete_actor->active_messages, consumer_index_index);
        (*bucket)--;
        --concrete_actor->active_requests;

        if (*bucket == concrete_actor->maximum_active_messages) {

            /*
             * The count was maximum_active_messages + 1
             * before removing 1 from it.
             */
            --concrete_actor->consumer_count_above_threshold;
        }

        if (!concrete_actor->forced) {
            bsal_assembly_block_classifier_verify(self, message);
        }
    }
}

void bsal_assembly_block_classifier_flush(struct thorium_actor *self, int customer_index, struct bsal_vector *buffers,
                int force)
{
    struct bsal_dna_kmer_frequency_block *customer_block_pointer;
    struct bsal_assembly_block_classifier *concrete_actor;
    int count;
    void *buffer;
    struct thorium_message message;
    int customer;
    struct bsal_memory_pool *ephemeral_memory;
    int threshold;
    int *bucket;

    /*
     * Only flush when required.
     */
    threshold = -1;

    concrete_actor = (struct bsal_assembly_block_classifier *)thorium_actor_concrete_actor(self);

    ephemeral_memory = thorium_actor_get_ephemeral_memory(self);
    customer = bsal_vector_at_as_int(&concrete_actor->consumers, customer_index);
    customer_block_pointer = (struct bsal_dna_kmer_frequency_block *)bsal_vector_at(buffers, customer_index);

    BSAL_DEBUGGER_ASSERT(customer_block_pointer != NULL);

    count = bsal_dna_kmer_frequency_block_pack_size(customer_block_pointer,
                    &concrete_actor->codec);

    /*
     * Don't flush if the force parameter is not set and there are not enough
     * bytes.
     */
    if (!force && count < threshold) {
        return;

    }

#ifdef BSAL_ASSEMBLY_BLOCK_CLASSIFIER_DEBUG_FLUSHING
    printf("DEBUG bsal_assembly_block_classifier_flush actual %d threshold %d\n", count,
                    threshold);
#endif

    buffer = bsal_memory_pool_allocate(ephemeral_memory, count);
    bsal_dna_kmer_frequency_block_pack(customer_block_pointer, buffer,
                    &concrete_actor->codec);

    thorium_message_init(&message, ACTION_PUSH_KMER_BLOCK, count, buffer);
    thorium_actor_send(self, customer, &message);

    bucket = bsal_vector_at(&concrete_actor->active_messages, customer_index);
    (*bucket)++;
    ++concrete_actor->active_requests;

    /*
     * Increment event counter
     */
    if (*bucket > concrete_actor->maximum_active_messages) {
        ++concrete_actor->consumer_count_above_threshold;
    }

    thorium_message_destroy(&message);
    bsal_memory_pool_free(ephemeral_memory, buffer);

    buffer = NULL;

    if (concrete_actor->flushed % 50000 == 0) {
        printf("assembly_block_classifier %d flushed %d blocks so far\n",
                        thorium_actor_name(self), concrete_actor->flushed);
    }

    /* Reset the buffer
     */

    bsal_dna_kmer_frequency_block_destroy(customer_block_pointer, ephemeral_memory);

    bsal_dna_kmer_frequency_block_init(customer_block_pointer, concrete_actor->kmer_length,
                    ephemeral_memory, &concrete_actor->codec,
                        concrete_actor->customer_block_size);

    concrete_actor->flushed++;
}

void bsal_assembly_block_classifier_verify(struct thorium_actor *self, struct thorium_message *message)
{
    struct bsal_assembly_block_classifier *concrete_actor;
    int producer_index;
    int producer;
#if 0
    int consumer_index;
#endif

    concrete_actor = (struct bsal_assembly_block_classifier *)thorium_actor_concrete_actor(self);

    /* Only continue if there are not too many
     * active messages.
     */

#if 0
    size = bsal_vector_size(&concrete_actor->active_messages);

    for (consumer_index = 0; consumer_index < size; consumer_index++) {
        bucket = (int *)bsal_vector_at(&concrete_actor->active_messages, consumer_index);

        /*
         * Not ready yet because the maximum is still
         * being used.
         */
        if (*bucket >= concrete_actor->maximum_active_messages) {
            return;
        }
    }
#endif

    if (concrete_actor->consumer_count_above_threshold > 0) {
        return;
    }

    /*
     * The code here is to make sure that there is enough memory.
     * If there is 0 active requests, then a reply must be sent
     * anyway even if the system is out of memory.
     */
    if (concrete_actor->active_requests > 0
                    && !bsal_memory_has_enough_bytes()) {
        return;
    }

    while (bsal_fast_queue_dequeue(&concrete_actor->stalled_producers, &producer_index)) {
        /* tell the producer to continue...
         */
        producer = producer_index;
        thorium_actor_send_empty(self, producer, ACTION_AGGREGATE_KERNEL_OUTPUT_REPLY);
    }
}

void bsal_assembly_block_classifier_aggregate_kernel_output(struct thorium_actor *self, struct thorium_message *message)
{
    int i;
    struct bsal_assembly_block_classifier *concrete_actor;
    struct bsal_vector buffers;
    int producer_index;
    int customer_count;
    struct bsal_dna_kmer_frequency_block *customer_block_pointer;
    int entries;
    struct bsal_dna_kmer_block input_block;
    struct bsal_dna_kmer_frequency_block *output_block;
    struct bsal_vector *kmers;
    struct bsal_memory_pool *ephemeral_memory;
    struct bsal_dna_kmer *kmer;
    int source;
    void *buffer;
    int customer_index;
    struct bsal_vector_iterator iterator;

    concrete_actor = (struct bsal_assembly_block_classifier *)thorium_actor_concrete_actor(self);

    if (bsal_vector_size(&concrete_actor->consumers) == 0) {
        printf("Error: classifier %d has no configured buffers\n",
                        thorium_actor_name(self));
        return;
    }

    ephemeral_memory = thorium_actor_get_ephemeral_memory(self);
    source = thorium_message_source(message);
    buffer = thorium_message_buffer(message);

    bsal_vector_init(&buffers, sizeof(struct bsal_dna_kmer_frequency_block));

    bsal_vector_resize(&buffers,
                    bsal_vector_size(&concrete_actor->consumers));

    /* enqueue the producer
     */
    producer_index = source;
    bsal_fast_queue_enqueue(&concrete_actor->stalled_producers, &producer_index);


#ifdef BSAL_ASSEMBLY_BLOCK_CLASSIFIER_DEBUG
    BSAL_DEBUG_MARKER("assembly_block_classifier receives");
    printf("name %d source %d UNPACK ON %d bytes\n",
                        thorium_actor_name(self), source, thorium_message_count(message));
#endif

    concrete_actor->received++;

    bsal_dna_kmer_block_init_empty(&input_block);
    bsal_dna_kmer_block_unpack(&input_block, buffer, ephemeral_memory,
                        &concrete_actor->codec);

#ifdef BSAL_ASSEMBLY_BLOCK_CLASSIFIER_DEBUG
        BSAL_DEBUG_MARKER("assembly_block_classifier before loop");
#endif

    /*
     * classify the kmers according to their ownership
     */

    kmers = bsal_dna_kmer_block_kmers(&input_block);
    entries = bsal_vector_size(kmers);

    customer_count = bsal_vector_size(&concrete_actor->consumers);
    concrete_actor->customer_block_size = (entries / customer_count) * 2;

    /*
     * Reserve entries
     */

    for (i = 0; i < bsal_vector_size(&concrete_actor->consumers); i++) {

        customer_block_pointer = (struct bsal_dna_kmer_frequency_block *)bsal_vector_at(&buffers,
                        i);
        bsal_dna_kmer_frequency_block_init(customer_block_pointer, concrete_actor->kmer_length,
                        ephemeral_memory, &concrete_actor->codec,
                        concrete_actor->customer_block_size);

    }

    for (i = 0; i < entries; i++) {
        kmer = (struct bsal_dna_kmer *)bsal_vector_at(kmers, i);

        /*
        bsal_dna_kmer_print(kmer);
        bsal_dna_kmer_length(kmer);
        */

        customer_index = bsal_dna_kmer_store_index(kmer, customer_count, concrete_actor->kmer_length,
                        &concrete_actor->codec, thorium_actor_get_ephemeral_memory(self));

        customer_block_pointer = (struct bsal_dna_kmer_frequency_block *)bsal_vector_at(&buffers,
                        customer_index);

#ifdef BSAL_ASSEMBLY_BLOCK_CLASSIFIER_DEBUG
        printf("DEBUG customer_index %d block pointer %p\n",
                        customer_index, (void *)customer_block_pointer);

        BSAL_DEBUG_MARKER("assembly_block_classifier before add");
#endif

        /*
         * add kmer to buffer and try to flush
         */

        /* classify the kmer and put it in the good buffer.
         */
        bsal_dna_kmer_frequency_block_add_kmer(customer_block_pointer, kmer, ephemeral_memory,
                        &concrete_actor->codec);


        /*
         * The Flush() action below is only required when using
         * bsal_dna_kmer_block.
         * bsal_dna_kmer_frequency_block does not require this
         * flush operation.
         */
#if 0
        /* Flush if necessary to avoid having very big buffers
         */
        if (0 && i % 32 == 0) {
            bsal_assembly_block_classifier_flush(self, customer_index, &buffers, 0);
        }
#endif

#ifdef BSAL_ASSEMBLY_BLOCK_CLASSIFIER_DEBUG
        BSAL_DEBUG_MARKER("assembly_block_classifier before flush");
#endif

    }

#ifdef BSAL_ASSEMBLY_BLOCK_CLASSIFIER_DEBUG
    BSAL_DEBUG_MARKER("assembly_block_classifier after loop");
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
    bsal_dna_kmer_block_destroy(&input_block, thorium_actor_get_ephemeral_memory(self));

#ifdef BSAL_ASSEMBLY_BLOCK_CLASSIFIER_DEBUG
        BSAL_DEBUG_MARKER("assembly_block_classifier marker EXIT");
#endif

    bsal_vector_iterator_init(&iterator, &buffers);

    /* Flush blocks.
     * Destroy blocks and
     * Destroy persistent memory pools, if any.
     */

    i = 0;
    while (bsal_vector_iterator_has_next(&iterator)) {

        bsal_vector_iterator_next(&iterator, (void **)&output_block);

        customer_index = i;

#ifdef BSAL_ASSEMBLY_BLOCK_CLASSIFIER_DEBUG
        printf("assembly_block_classifier flushing %d\n", customer_index);
#endif

        bsal_assembly_block_classifier_flush(self, customer_index, &buffers, 1);

        /*
         * Destroy block
         */
        bsal_dna_kmer_frequency_block_destroy(output_block, ephemeral_memory);

        i++;
    }

    bsal_vector_iterator_destroy(&iterator);
    bsal_vector_destroy(&buffers);

    bsal_assembly_block_classifier_verify(self, message);
}

void bsal_assembly_block_classifier_pack_message(struct thorium_actor *actor, struct thorium_message *message)
{
    void *new_buffer;
    int new_count;
    struct bsal_memory_pool *ephemeral_memory;
    struct thorium_message new_message;

    ephemeral_memory = thorium_actor_get_ephemeral_memory(actor);
    new_count = bsal_assembly_block_classifier_pack_size(actor);
    new_buffer = bsal_memory_pool_allocate(ephemeral_memory, new_count);

    bsal_assembly_block_classifier_pack(actor, new_buffer);

    thorium_message_init(&new_message, ACTION_PACK_REPLY, new_count, new_buffer);
    thorium_actor_send_reply(actor, &new_message);
    thorium_message_destroy(&new_message);

    bsal_memory_pool_free(ephemeral_memory, new_buffer);
    new_buffer = NULL;
}

void bsal_assembly_block_classifier_unpack_message(struct thorium_actor *actor, struct thorium_message *message)
{
    void *buffer;

    buffer = thorium_message_buffer(message);

    bsal_assembly_block_classifier_unpack(actor, buffer);

    thorium_actor_send_reply_empty(actor, ACTION_UNPACK_REPLY);
}

int bsal_assembly_block_classifier_set_consumers(struct thorium_actor *actor, void *buffer)
{
    struct bsal_assembly_block_classifier *concrete_actor;
    int bytes;
    int size;
    int i;
    int zero;

    concrete_actor = (struct bsal_assembly_block_classifier *)thorium_actor_concrete_actor(actor);

    /*
     * receive customer list
     */
    bsal_vector_init(&concrete_actor->consumers, 0);
    bytes = bsal_vector_unpack(&concrete_actor->consumers, buffer);

    size = bsal_vector_size(&concrete_actor->consumers);

    bsal_vector_resize(&concrete_actor->active_messages, size);

    zero = 0;

    for (i = 0; i < size; i++) {
        bsal_vector_set(&concrete_actor->active_messages, i, &zero);
    }

    /*
     * The maximum number of active messages for any consumer is
     * set here.
     */
    concrete_actor->maximum_active_messages = thorium_actor_active_message_limit(actor);

    printf("DEBUG45 classifier %d preparing %d buffers, kmer_length %d, ACTIVE_MESSAGE_LIMIT %d\n",
                    thorium_actor_name(actor),
                        (int)bsal_vector_size(&concrete_actor->consumers),
                        concrete_actor->kmer_length,
                        concrete_actor->maximum_active_messages);

#ifdef BSAL_ASSEMBLY_BLOCK_CLASSIFIER_DEBUG
    printf("DEBUG classifier configured %d consumers\n",
                        (int)bsal_vector_size(&concrete_actor->consumers));
#endif

    return bytes;
}

int bsal_assembly_block_classifier_pack_unpack(struct thorium_actor *actor, int operation, void *buffer)
{
    struct bsal_packer packer;
    int bytes;
    struct bsal_assembly_block_classifier *concrete_actor;

    concrete_actor = (struct bsal_assembly_block_classifier *)thorium_actor_concrete_actor(actor);

    bytes = 0;

    bsal_packer_init(&packer, operation, buffer);

    /*
     * Pack the kmer length
     */
    bsal_packer_process(&packer, &concrete_actor->kmer_length, sizeof(concrete_actor->kmer_length));

    bytes += bsal_packer_get_byte_count(&packer);

    /*
    if (operation == BSAL_PACKER_OPERATION_UNPACK) {
        printf("assembly_block_classifier %d unpacked kmer length %d\n",
                        thorium_actor_name(actor),
                        concrete_actor->kmer_length);
    }
*/
    /* Pack the consumers
     */

    if (operation == BSAL_PACKER_OPERATION_UNPACK) {
        bsal_assembly_block_classifier_set_consumers(actor,
                        ((char *)buffer) + bytes);

    } else {

        bytes += bsal_vector_pack_unpack(&concrete_actor->consumers,
                        (char *)buffer + bytes,
                        operation);
    }

    bsal_packer_destroy(&packer);

    return bytes;
}

int bsal_assembly_block_classifier_pack(struct thorium_actor *actor, void *buffer)
{
    return bsal_assembly_block_classifier_pack_unpack(actor, BSAL_PACKER_OPERATION_PACK, buffer);
}

int bsal_assembly_block_classifier_unpack(struct thorium_actor *actor, void *buffer)
{
    return bsal_assembly_block_classifier_pack_unpack(actor, BSAL_PACKER_OPERATION_UNPACK, buffer);
}

int bsal_assembly_block_classifier_pack_size(struct thorium_actor *actor)
{
    return bsal_assembly_block_classifier_pack_unpack(actor, BSAL_PACKER_OPERATION_DRY_RUN, NULL);
}


