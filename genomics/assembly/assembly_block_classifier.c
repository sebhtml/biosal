
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


struct bsal_script bsal_assembly_block_classifier_script = {
    .identifier = BSAL_ASSEMBLY_BLOCK_CLASSIFIER_SCRIPT,
    .name = "bsal_assembly_block_classifier",
    .description = "",
    .version = "",
    .author = "Sebastien Boisvert",
    .size = sizeof(struct bsal_assembly_block_classifier),
    .init = bsal_assembly_block_classifier_init,
    .destroy = bsal_assembly_block_classifier_destroy,
    .receive = bsal_assembly_block_classifier_receive
};

void bsal_assembly_block_classifier_init(struct bsal_actor *self)
{
    struct bsal_assembly_block_classifier *concrete_actor;

    concrete_actor = (struct bsal_assembly_block_classifier *)bsal_actor_concrete_actor(self);
    concrete_actor->received = 0;
    concrete_actor->last = 0;

    bsal_vector_init(&concrete_actor->consumers, sizeof(int));

    concrete_actor->flushed = 0;

    bsal_dna_codec_init(&concrete_actor->codec);
    if (bsal_actor_get_node_count(self) >= BSAL_DNA_CODEC_MINIMUM_NODE_COUNT_FOR_TWO_BIT) {
#ifdef BSAL_DNA_CODEC_USE_TWO_BIT_ENCODING_FOR_TRANSPORT
        bsal_dna_codec_enable_two_bit_encoding(&concrete_actor->codec);
#endif
    }

    bsal_ring_queue_init(&concrete_actor->stalled_producers, sizeof(int));
    bsal_vector_init(&concrete_actor->active_messages, sizeof(int));

    concrete_actor->forced = 0;

    bsal_actor_add_route(self, BSAL_AGGREGATE_KERNEL_OUTPUT,
                    bsal_assembly_block_classifier_aggregate_kernel_output);

    /* Enable cloning stuff
     */
    bsal_actor_send_to_self_empty(self, BSAL_ACTOR_PACK_ENABLE);

    bsal_actor_add_route(self, BSAL_ACTOR_PACK,
                    bsal_assembly_block_classifier_pack_message);
    bsal_actor_add_route(self, BSAL_ACTOR_UNPACK,
                    bsal_assembly_block_classifier_unpack_message);

    printf("assembly_block_classifier %d is online\n", bsal_actor_name(self));
}

void bsal_assembly_block_classifier_destroy(struct bsal_actor *self)
{
    struct bsal_assembly_block_classifier *concrete_actor;

    concrete_actor = (struct bsal_assembly_block_classifier *)bsal_actor_concrete_actor(self);

    bsal_vector_destroy(&concrete_actor->active_messages);
    concrete_actor->received = 0;
    concrete_actor->last = 0;
    bsal_dna_codec_destroy(&concrete_actor->codec);

    bsal_ring_queue_destroy(&concrete_actor->stalled_producers);

    bsal_vector_destroy(&concrete_actor->consumers);

}

void bsal_assembly_block_classifier_receive(struct bsal_actor *self, struct bsal_message *message)
{
    int tag;
    struct bsal_assembly_block_classifier *concrete_actor;
    void *buffer;
    int source;
    int consumer_index_index;
    int *bucket;

    if (bsal_actor_call_handler(self, message)) {
        return;
    }

    concrete_actor = (struct bsal_assembly_block_classifier *)bsal_actor_concrete_actor(self);
    buffer = bsal_message_buffer(message);
    tag = bsal_message_tag(message);
    source = bsal_message_source(message);

    if (tag == BSAL_ACTOR_ASK_TO_STOP) {

        bsal_actor_ask_to_stop(self, message);

    } else if (tag == BSAL_SET_KMER_LENGTH) {

        bsal_message_helper_unpack_int(message, 0, &concrete_actor->kmer_length);

        bsal_actor_send_reply_empty(self, BSAL_SET_KMER_LENGTH_REPLY);

    } else if (tag == BSAL_ACTOR_SET_CONSUMERS) {

        bsal_assembly_block_classifier_set_consumers(self, buffer);

        bsal_actor_send_reply_empty(self, BSAL_ACTOR_SET_CONSUMERS_REPLY);

    } else if (tag == BSAL_AGGREGATOR_FLUSH) {


        concrete_actor->forced = 1;

        bsal_actor_send_reply_empty(self, BSAL_AGGREGATOR_FLUSH_REPLY);

    } else if (tag == BSAL_PUSH_KMER_BLOCK_REPLY) {

#ifdef BSAL_ASSEMBLY_BLOCK_CLASSIFIER_DEBUG
        printf("BEFORE BSAL_PUSH_KMER_BLOCK_REPLY %d\n", concrete_actor->active_messages);
#endif

        consumer_index_index = bsal_vector_index_of(&concrete_actor->consumers, &source);

        bucket = (int *)bsal_vector_at(&concrete_actor->active_messages, consumer_index_index);

        (*bucket)--;

        if (!concrete_actor->forced) {
            bsal_assembly_block_classifier_verify(self, message);
        }
    }
}

void bsal_assembly_block_classifier_flush(struct bsal_actor *self, int customer_index, struct bsal_vector *buffers,
                int force)
{
    struct bsal_dna_kmer_frequency_block *customer_block_pointer;
    struct bsal_assembly_block_classifier *concrete_actor;
    int count;
    void *buffer;
    struct bsal_message message;
    int customer;
    struct bsal_memory_pool *ephemeral_memory;
    int threshold;
    int *bucket;

    /*
     * Only flush when required
     */
    threshold = BSAL_SEQUENCE_STORE_FINAL_BLOCK_SIZE * 3;

    concrete_actor = (struct bsal_assembly_block_classifier *)bsal_actor_concrete_actor(self);

    ephemeral_memory = bsal_actor_get_ephemeral_memory(self);
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

    bsal_message_init(&message, BSAL_PUSH_KMER_BLOCK, count, buffer);
    bsal_actor_send(self, customer, &message);

    bucket = (int *)bsal_vector_at(&concrete_actor->active_messages, customer_index);
    (*bucket)++;

    bsal_message_destroy(&message);
    bsal_memory_pool_free(ephemeral_memory, buffer);

    buffer = NULL;

    if (concrete_actor->flushed % 50000 == 0) {
        printf("assembly_block_classifier %d flushed %d blocks so far\n",
                        bsal_actor_name(self), concrete_actor->flushed);
    }

    /* Reset the buffer
     */

    bsal_dna_kmer_frequency_block_destroy(customer_block_pointer, ephemeral_memory);

    bsal_dna_kmer_frequency_block_init(customer_block_pointer, concrete_actor->kmer_length,
                    ephemeral_memory, &concrete_actor->codec,
                        concrete_actor->customer_block_size);

    concrete_actor->flushed++;
}

void bsal_assembly_block_classifier_verify(struct bsal_actor *self, struct bsal_message *message)
{
    struct bsal_assembly_block_classifier *concrete_actor;
    int producer_index;
    int producer;
    int consumer_index;
    int size;
    int *bucket;

    concrete_actor = (struct bsal_assembly_block_classifier *)bsal_actor_concrete_actor(self);

    /* Only continue if there are not too many
     * active messages.
     */

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

    while (bsal_ring_queue_dequeue(&concrete_actor->stalled_producers, &producer_index)) {
        /* tell the producer to continue...
         */
        producer = producer_index;
        bsal_actor_send_empty(self, producer, BSAL_AGGREGATE_KERNEL_OUTPUT_REPLY);
    }
}

void bsal_assembly_block_classifier_aggregate_kernel_output(struct bsal_actor *self, struct bsal_message *message)
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

    concrete_actor = (struct bsal_assembly_block_classifier *)bsal_actor_concrete_actor(self);

    if (bsal_vector_size(&concrete_actor->consumers) == 0) {
        printf("Error: classifier %d has no configured buffers\n",
                        bsal_actor_name(self));
        return;
    }

    ephemeral_memory = bsal_actor_get_ephemeral_memory(self);
    source = bsal_message_source(message);
    buffer = bsal_message_buffer(message);

    bsal_vector_init(&buffers, sizeof(struct bsal_dna_kmer_frequency_block));

    bsal_vector_resize(&buffers,
                    bsal_vector_size(&concrete_actor->consumers));

    /* enqueue the producer
     */
    producer_index = source;
    bsal_ring_queue_enqueue(&concrete_actor->stalled_producers, &producer_index);


#ifdef BSAL_ASSEMBLY_BLOCK_CLASSIFIER_DEBUG
    BSAL_DEBUG_MARKER("assembly_block_classifier receives");
    printf("name %d source %d UNPACK ON %d bytes\n",
                        bsal_actor_name(self), source, bsal_message_count(message));
#endif

    concrete_actor->received++;

    bsal_dna_kmer_block_unpack(&input_block, buffer, bsal_actor_get_ephemeral_memory(self),
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
                        &concrete_actor->codec, bsal_actor_get_ephemeral_memory(self));

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
                        bsal_actor_name(self),
                        concrete_actor->received);

        concrete_actor->last = concrete_actor->received;
    }

    /* destroy the local copy of the block
     */
    bsal_dna_kmer_block_destroy(&input_block, bsal_actor_get_ephemeral_memory(self));

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

void bsal_assembly_block_classifier_pack_message(struct bsal_actor *actor, struct bsal_message *message)
{
    void *new_buffer;
    int new_count;
    struct bsal_memory_pool *ephemeral_memory;
    struct bsal_message new_message;

    ephemeral_memory = bsal_actor_get_ephemeral_memory(actor);
    new_count = bsal_assembly_block_classifier_pack_size(actor);
    new_buffer = bsal_memory_pool_allocate(ephemeral_memory, new_count);

    bsal_assembly_block_classifier_pack(actor, new_buffer);

    bsal_message_init(&new_message, BSAL_ACTOR_PACK_REPLY, new_count, new_buffer);
    bsal_actor_send_reply(actor, &new_message);
    bsal_message_destroy(&new_message);

    bsal_memory_pool_free(ephemeral_memory, new_buffer);
    new_buffer = NULL;
}

void bsal_assembly_block_classifier_unpack_message(struct bsal_actor *actor, struct bsal_message *message)
{
    void *buffer;

    buffer = bsal_message_buffer(message);

    bsal_assembly_block_classifier_unpack(actor, buffer);

    bsal_actor_send_reply_empty(actor, BSAL_ACTOR_UNPACK_REPLY);
}

int bsal_assembly_block_classifier_set_consumers(struct bsal_actor *actor, void *buffer)
{
    struct bsal_assembly_block_classifier *concrete_actor;
    int bytes;
    int size;
    int i;
    int zero;

    concrete_actor = (struct bsal_assembly_block_classifier *)bsal_actor_concrete_actor(actor);

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
    concrete_actor->maximum_active_messages = 1;

    /* Increase the active message count if running on one node */
    if (bsal_actor_get_node_count(actor) == 1) {

        /*
         * If the thing runs on one single node, there is no transport
         * calls so everything should be regular.
         */
        concrete_actor->maximum_active_messages = 4;
    }

    printf("DEBUG45 classifier %d preparing %d buffers, kmer_length %d\n",
                    bsal_actor_name(actor),
                        (int)bsal_vector_size(&concrete_actor->consumers),
                        concrete_actor->kmer_length);

#ifdef BSAL_ASSEMBLY_BLOCK_CLASSIFIER_DEBUG
    printf("DEBUG classifier configured %d consumers\n",
                        (int)bsal_vector_size(&concrete_actor->consumers));
#endif

    return bytes;
}

int bsal_assembly_block_classifier_pack_unpack(struct bsal_actor *actor, int operation, void *buffer)
{
    struct bsal_packer packer;
    int bytes;
    struct bsal_assembly_block_classifier *concrete_actor;

    concrete_actor = (struct bsal_assembly_block_classifier *)bsal_actor_concrete_actor(actor);

    bytes = 0;

    bsal_packer_init(&packer, operation, buffer);

    /*
     * Pack the kmer length
     */
    bsal_packer_work(&packer, &concrete_actor->kmer_length, sizeof(concrete_actor->kmer_length));

    bytes += bsal_packer_worked_bytes(&packer);

    /*
    if (operation == BSAL_PACKER_OPERATION_UNPACK) {
        printf("assembly_block_classifier %d unpacked kmer length %d\n",
                        bsal_actor_name(actor),
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

int bsal_assembly_block_classifier_pack(struct bsal_actor *actor, void *buffer)
{
    return bsal_assembly_block_classifier_pack_unpack(actor, BSAL_PACKER_OPERATION_PACK, buffer);
}

int bsal_assembly_block_classifier_unpack(struct bsal_actor *actor, void *buffer)
{
    return bsal_assembly_block_classifier_pack_unpack(actor, BSAL_PACKER_OPERATION_UNPACK, buffer);
}

int bsal_assembly_block_classifier_pack_size(struct bsal_actor *actor)
{
    return bsal_assembly_block_classifier_pack_unpack(actor, BSAL_PACKER_OPERATION_DRY_RUN, NULL);
}


