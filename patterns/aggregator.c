
#include "aggregator.h"

#include <data/dna_kmer_block.h>
#include <data/dna_kmer.h>

#include <kernels/dna_kmer_counter_kernel.h>

#include <helpers/actor_helper.h>
#include <helpers/message_helper.h>
#include <helpers/vector_helper.h>

#include <storage/kmer_store.h>

#include <structures/vector_iterator.h>

#include <system/debugger.h>
#include <system/memory.h>

#include <stdio.h>
#include <inttypes.h>

/* debugging options
 */
/*
#define BSAL_AGGREGATOR_DEBUG
*/

/* aggregator runtime constants
 */

#define BSAL_AGGREGATOR_FORCED_FLUSH_NO 0
#define BSAL_AGGREGATOR_FORCED_FLUSH_YES 1

struct bsal_script bsal_aggregator_script = {
    .name = BSAL_AGGREGATOR_SCRIPT,
    .init = bsal_aggregator_init,
    .destroy = bsal_aggregator_destroy,
    .receive = bsal_aggregator_receive,
    .size = sizeof(struct bsal_aggregator),
    .description = "aggregator"
};

void bsal_aggregator_init(struct bsal_actor *self)
{
    struct bsal_aggregator *concrete_actor;

    concrete_actor = (struct bsal_aggregator *)bsal_actor_concrete_actor(self);
    concrete_actor->received = 0;
    concrete_actor->last = 0;

    bsal_vector_init(&concrete_actor->customers, sizeof(int));
    bsal_vector_init(&concrete_actor->buffers, sizeof(struct bsal_dna_kmer_block));

    concrete_actor->customer_block_size = 2048;
    concrete_actor->flushed = 0;

    bsal_dna_codec_init(&concrete_actor->codec);
}

void bsal_aggregator_destroy(struct bsal_actor *self)
{
    struct bsal_aggregator *concrete_actor;
    struct bsal_dna_kmer_block *block;
    struct bsal_vector_iterator iterator;

    concrete_actor = (struct bsal_aggregator *)bsal_actor_concrete_actor(self);
    concrete_actor->received = 0;
    concrete_actor->last = 0;
    bsal_dna_codec_destroy(&concrete_actor->codec);

    bsal_vector_iterator_init(&iterator, &concrete_actor->buffers);

    while (bsal_vector_iterator_has_next(&iterator)) {

        bsal_vector_iterator_next(&iterator, (void **)&block);

        /* TODO:
         * Destroy block
        break;
         */
        bsal_dna_kmer_block_destroy(block);
    }

    bsal_vector_iterator_destroy(&iterator);

    bsal_vector_destroy(&concrete_actor->customers);
    bsal_vector_destroy(&concrete_actor->buffers);
}

void bsal_aggregator_receive(struct bsal_actor *self, struct bsal_message *message)
{
    int tag;
    struct bsal_aggregator *concrete_actor;
    void *buffer;
    int source;
    struct bsal_dna_kmer_block block;
    int source_index;
    struct bsal_vector *kmers;
    struct bsal_dna_kmer *kmer;
    int entries;
    int i;
    struct bsal_vector customers;
    struct bsal_dna_kmer_block *customer_block_pointer;
    int customer_index;
    int customer_count;

    concrete_actor = (struct bsal_aggregator *)bsal_actor_concrete_actor(self);
    buffer = bsal_message_buffer(message);
    tag = bsal_message_tag(message);
    source = bsal_message_source(message);

    if (tag == BSAL_AGGREGATE_KERNEL_OUTPUT) {

        if (bsal_vector_size(&concrete_actor->buffers) == 0) {
            printf("aggregator actor/%d: Error, no configured buffers\n",
                            bsal_actor_name(self));
            return;
        }

#ifdef BSAL_AGGREGATOR_DEBUG
        BSAL_DEBUG_MARKER("aggregator receives");
        printf("name %d source %d UNPACK ON %d bytes\n",
                        bsal_actor_name(self), source, bsal_message_count(message));
#endif

        concrete_actor->received++;

        bsal_dna_kmer_block_unpack(&block, buffer);

#ifdef BSAL_AGGREGATOR_DEBUG
        BSAL_DEBUG_MARKER("aggregator before loop");
#endif

        /* TODO
         * classify the kmers according to their ownership
         */

        kmers = bsal_dna_kmer_block_kmers(&block);
        entries = bsal_vector_size(kmers);

        customer_count = bsal_vector_size(&concrete_actor->customers);

        for (i = 0; i < entries; i++) {
            kmer = (struct bsal_dna_kmer *)bsal_vector_at(kmers, i);

            /*
            bsal_dna_kmer_print(kmer);
            bsal_dna_kmer_length(kmer);
            */

            customer_index = bsal_dna_kmer_store_index(kmer, customer_count, concrete_actor->kmer_length,
                            &concrete_actor->codec);

            customer_block_pointer = (struct bsal_dna_kmer_block *)bsal_vector_at(&concrete_actor->buffers,
                            customer_index);

#ifdef BSAL_AGGREGATOR_DEBUG
            printf("DEBUG customer_index %d block pointer %p\n",
                            customer_index, (void *)customer_block_pointer);

            BSAL_DEBUG_MARKER("aggregator before add");
#endif

            /*
             * add kmer to buffer and try to flush
             */

            bsal_dna_kmer_block_add_kmer(customer_block_pointer, kmer);

#ifdef BSAL_AGGREGATOR_DEBUG
            BSAL_DEBUG_MARKER("aggregator before flush");
#endif

            bsal_aggregator_flush(self, customer_index, BSAL_AGGREGATOR_FORCED_FLUSH_NO);

            /* classify the kmer and put it in the good buffer.
             */
        }

#ifdef BSAL_AGGREGATOR_DEBUG
        BSAL_DEBUG_MARKER("aggregator after loop");
#endif

        source_index = bsal_dna_kmer_block_source_index(&block);

        if (concrete_actor->last == 0
                        || concrete_actor->received >= concrete_actor->last + 1000) {

            printf("aggregator/%d received %" PRIu64 " kernel outputs so far\n",
                            bsal_actor_name(self),
                            concrete_actor->received);

            concrete_actor->last = concrete_actor->received;
        }

        /* answer immediately
         */
        bsal_actor_helper_send_reply_int(self, BSAL_AGGREGATE_KERNEL_OUTPUT_REPLY,
                        source_index);

        /* destroy the local copy of the block
         */
        bsal_dna_kmer_block_destroy(&block);

#ifdef BSAL_AGGREGATOR_DEBUG
        BSAL_DEBUG_MARKER("aggregator marker EXIT");
#endif

    } else if (tag == BSAL_ACTOR_ASK_TO_STOP
                    && source == bsal_actor_supervisor(self)) {

        bsal_actor_helper_send_to_self_empty(self, BSAL_ACTOR_STOP);

    } else if (tag == BSAL_SET_KMER_LENGTH) {

        bsal_message_helper_unpack_int(message, 0, &concrete_actor->kmer_length);

        bsal_actor_helper_send_reply_empty(self, BSAL_SET_KMER_LENGTH_REPLY);

    } else if (tag == BSAL_ACTOR_SET_CONSUMERS) {

        /*
         * receive customer list
         */
        bsal_vector_unpack(&customers, buffer);
        bsal_actor_helper_add_acquaintances(self, &customers, &concrete_actor->customers);

        printf("DEBUG45 preparing %d buffers, kmer_length %d, block size %d\n",
                        (int)bsal_vector_size(&concrete_actor->customers),
                        concrete_actor->kmer_length,
                        concrete_actor->customer_block_size);

        bsal_vector_resize(&concrete_actor->buffers,
                        bsal_vector_size(&concrete_actor->customers));

        for (i = 0; i < bsal_vector_size(&concrete_actor->customers); i++) {

            customer_block_pointer = (struct bsal_dna_kmer_block *)bsal_vector_at(&concrete_actor->buffers,
                            i);
            bsal_dna_kmer_block_init(customer_block_pointer, concrete_actor->kmer_length, -1,
                            concrete_actor->customer_block_size);

        }

#ifdef BSAL_AGGREGATOR_DEBUG
        printf("DEBUG aggregator configured %d customers\n",
                        (int)bsal_vector_size(&concrete_actor->customers));
#endif

        bsal_actor_helper_send_reply_empty(self, BSAL_ACTOR_SET_CONSUMERS_REPLY);

    } else if (tag == BSAL_AGGREGATOR_FLUSH) {

        customer_count = bsal_vector_size(&concrete_actor->customers);

        for (customer_index = 0; customer_index < customer_count; customer_index++) {

            bsal_aggregator_flush(self, customer_index, BSAL_AGGREGATOR_FORCED_FLUSH_YES);
        }

        bsal_actor_helper_send_reply_empty(self, BSAL_AGGREGATOR_FLUSH_REPLY);
    }
}

void bsal_aggregator_flush(struct bsal_actor *self, int customer_index, int forced_flush)
{
    struct bsal_dna_kmer_block *customer_block_pointer;
    int actual;
    int threshold;
    struct bsal_aggregator *concrete_actor;
    int count;
    void *buffer;
    struct bsal_message message;
    int customer;

    concrete_actor = (struct bsal_aggregator *)bsal_actor_concrete_actor(self);

    customer = bsal_actor_helper_get_acquaintance(self, &concrete_actor->customers, customer_index);
    threshold = concrete_actor->customer_block_size;
    customer_block_pointer = (struct bsal_dna_kmer_block *)bsal_vector_at(&concrete_actor->buffers, customer_index);
    actual = bsal_dna_kmer_block_size(customer_block_pointer);

#ifdef BSAL_AGGREGATOR_DEBUG
    printf("DEBUG bsal_aggregator_flush actual %d threshold %d\n", actual,
                    threshold);
#endif

    /*
     * Don't flush anything, the actual value is below
     * the threshold.
     */
    if (forced_flush == BSAL_AGGREGATOR_FORCED_FLUSH_NO
                    && actual < threshold) {
        return;
    }

    count = bsal_dna_kmer_block_pack_size(customer_block_pointer);
    buffer = bsal_malloc(count);
    bsal_dna_kmer_block_pack(customer_block_pointer, buffer);

    bsal_message_init(&message, BSAL_PUSH_KMER_BLOCK, count, buffer);
    bsal_actor_send(self, customer, &message);
    bsal_message_destroy(&message);
    bsal_free(buffer);

    bsal_dna_kmer_block_destroy(customer_block_pointer);

    bsal_dna_kmer_block_init(customer_block_pointer, concrete_actor->kmer_length, -1,
                            concrete_actor->customer_block_size);
    buffer = NULL;

    if (concrete_actor->flushed % 1000 == 0) {
        printf("aggregator actor/%d flushed %d blocks so far\n",
                        bsal_actor_name(self), concrete_actor->flushed);
    }

    concrete_actor->flushed++;
}
