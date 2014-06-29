
#ifndef BSAL_AGGREGATOR_H
#define BSAL_AGGREGATOR_H

#include <engine/actor.h>

#include <data/dna_codec.h>

#include <structures/vector.h>

#include <system/memory_pool.h>

#include <stdint.h>

#define BSAL_AGGREGATOR_SCRIPT 0x82673850

struct bsal_aggregator {
    uint64_t received;
    uint64_t last;
    int kmer_length;

    struct bsal_vector customers;
    struct bsal_vector buffers;

    int customer_block_size;
    int flushed;

    struct bsal_dna_codec codec;
    struct bsal_vector persistent_memory_pools;
};

/* message tags
 */
#define BSAL_AGGREGATE_KERNEL_OUTPUT 0x0000225f
#define BSAL_AGGREGATE_KERNEL_OUTPUT_REPLY 0x00005cf2
#define BSAL_AGGREGATOR_FLUSH 0x00007305
#define BSAL_AGGREGATOR_FLUSH_REPLY 0x000029fe

extern struct bsal_script bsal_aggregator_script;

void bsal_aggregator_init(struct bsal_actor *actor);
void bsal_aggregator_destroy(struct bsal_actor *actor);
void bsal_aggregator_receive(struct bsal_actor *actor, struct bsal_message *message);

void bsal_aggregator_flush(struct bsal_actor *self, int customer_index, int forced_flush);

#endif
