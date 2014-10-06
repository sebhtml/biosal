
#ifndef BIOSAL_AGGREGATOR_H
#define BIOSAL_AGGREGATOR_H

#include <engine/thorium/actor.h>

#include <genomics/data/dna_codec.h>

#include <core/structures/vector.h>
#include <core/structures/fast_queue.h>

#include <core/system/memory_pool.h>

#include <stdint.h>

#define SCRIPT_AGGREGATOR 0x82673850

struct biosal_aggregator {
    uint64_t received;
    uint64_t last;
    int kmer_length;

    struct core_vector consumers;

    struct core_fast_queue stalled_producers;

    int maximum_active_messages;
    struct core_vector active_messages;

    int customer_block_size;
    int flushed;

    int forced;

    struct biosal_dna_codec codec;
};

/* message tags
 */
#define ACTION_AGGREGATE_KERNEL_OUTPUT 0x0000225f
#define ACTION_AGGREGATE_KERNEL_OUTPUT_REPLY 0x00005cf2
#define ACTION_AGGREGATOR_FLUSH 0x00007305
#define ACTION_AGGREGATOR_FLUSH_REPLY 0x000029fe

extern struct thorium_script biosal_aggregator_script;

#endif
