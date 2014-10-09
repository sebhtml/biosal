
#ifndef BIOSAL_ASSEMBLY_BLOCK_CLASSIFIER_H
#define BIOSAL_ASSEMBLY_BLOCK_CLASSIFIER_H

#include <engine/thorium/actor.h>

#include <genomics/data/dna_codec.h>
#include <genomics/kernels/aggregator.h>

#include <core/structures/vector.h>
#include <core/structures/fast_queue.h>

#include <core/system/memory_pool.h>

#include <stdint.h>

#define SCRIPT_ASSEMBLY_BLOCK_CLASSIFIER 0x8f6f8767

/*
 * This is a classifier that dispatch a vertex block to the
 * graph stores.
 */
struct biosal_assembly_block_classifier {
    uint64_t received;
    uint64_t last;
    int kmer_length;

    struct core_memory_pool persistent_pool;
    struct core_vector consumers;

    struct core_fast_queue stalled_producers;

    int maximum_active_messages;
    struct core_vector active_messages;
    int active_requests;

    int customer_block_size;
    int flushed;

    int forced;

    struct biosal_dna_codec codec;

    int consumer_count_above_threshold;

    /*
     * bins
     */
    struct core_vector buffers;
};


extern struct thorium_script biosal_assembly_block_classifier_script;

#endif
