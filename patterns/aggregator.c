
#include "aggregator.h"

#include <data/dna_kmer_block.h>
#include <data/dna_kmer.h>

#include <kernels/kmer_counter_kernel.h>

#include <helpers/actor_helper.h>
#include <helpers/message_helper.h>

#include <system/debugger.h>

#include <stdio.h>
#include <inttypes.h>

/* debugging options
 */
/*
#define BSAL_AGGREGATOR_DEBUG
*/

struct bsal_script bsal_aggregator_script = {
    .name = BSAL_AGGREGATOR_SCRIPT,
    .init = bsal_aggregator_init,
    .destroy = bsal_aggregator_destroy,
    .receive = bsal_aggregator_receive,
    .size = sizeof(struct bsal_aggregator)
};

void bsal_aggregator_init(struct bsal_actor *actor)
{
    struct bsal_aggregator *concrete_actor;

    concrete_actor = (struct bsal_aggregator *)bsal_actor_concrete_actor(actor);
    concrete_actor->received = 0;
    concrete_actor->last = 0;
}

void bsal_aggregator_destroy(struct bsal_actor *actor)
{
    struct bsal_aggregator *concrete_actor;

    concrete_actor = (struct bsal_aggregator *)bsal_actor_concrete_actor(actor);
    concrete_actor->received = 0;
    concrete_actor->last = 0;
}

void bsal_aggregator_receive(struct bsal_actor *actor, struct bsal_message *message)
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

    concrete_actor = (struct bsal_aggregator *)bsal_actor_concrete_actor(actor);
    buffer = bsal_message_buffer(message);
    tag = bsal_message_tag(message);
    source = bsal_message_source(message);

    if (tag == BSAL_AGGREGATE_KERNEL_OUTPUT) {

#ifdef BSAL_AGGREGATOR_DEBUG
        BSAL_DEBUG_MARKER("aggregator receives");

        printf("name %d source %d UNPACK ON %d bytes\n",
                        bsal_actor_name(actor), source, bsal_message_count(message));
#endif

        concrete_actor->received++;

        bsal_dna_kmer_block_unpack(&block, buffer);

        /* TODO
         * classify the kmers according to their ownership
         */

        kmers = bsal_dna_kmer_block_kmers(&block);
        entries = bsal_vector_size(kmers);

        for (i = 0; i < entries; i++) {
            kmer = (struct bsal_dna_kmer *)bsal_vector_at(kmers, i);

            /*
            bsal_dna_kmer_print(kmer);
            */

            bsal_dna_kmer_length(kmer);

            /* classify the kmer and put it in the good buffer.
             */
        }

#ifdef BSAL_AGGREGATOR_DEBUG
        BSAL_DEBUG_MARKER("aggregator after unpack");
#endif

        source_index = bsal_dna_kmer_block_source_index(&block);

        if (concrete_actor->last == 0
                        || concrete_actor->received > concrete_actor->last + 10000) {

            printf("aggregator actor/%d received %" PRIu64 " kernel outputs so far.\n",
                            bsal_actor_name(actor),
                            concrete_actor->received);

            concrete_actor->last = concrete_actor->received;
        }

        /* answer immediately
         */
        bsal_actor_helper_send_reply_int(actor, BSAL_AGGREGATE_KERNEL_OUTPUT_REPLY,
                        source_index);

#ifdef BSAL_AGGREGATOR_DEBUG
        BSAL_DEBUG_MARKER("aggregator OK 2");
#endif

        /* destroy the local copy of the block
         */
        bsal_dna_kmer_block_destroy(&block);

    } else if (tag == BSAL_ACTOR_ASK_TO_STOP
                    && source == bsal_actor_supervisor(actor)) {

        bsal_actor_helper_send_to_self_empty(actor, BSAL_ACTOR_STOP);

    } else if (tag == BSAL_SET_KMER_LENGTH) {

        bsal_message_helper_unpack_int(message, 0, &concrete_actor->kmer_length);

        bsal_actor_helper_send_reply_empty(actor, BSAL_SET_KMER_LENGTH_REPLY);
    }

}
