
#ifndef BIOSAL_ASSEMBLY_ARC_CLASSIFIER_H
#define BIOSAL_ASSEMBLY_ARC_CLASSIFIER_H

#include <engine/thorium/actor.h>

#include <genomics/data/dna_codec.h>
#include <core/structures/vector.h>

#include <core/system/memory_pool.h>

#define SCRIPT_ASSEMBLY_ARC_CLASSIFIER 0x115b87ed

/*
 * This actor classifies arcs.
 */
struct biosal_assembly_arc_classifier {
    int kmer_length;

    int source;
    struct core_vector consumers;

    struct biosal_dna_codec codec;

    int received_blocks;

    struct core_vector pending_requests;
    int active_requests;
    int producer_is_waiting;
    int maximum_pending_request_count;

    int consumer_count_above_threshold;

    /*
     * Output bins
     */
    struct core_memory_pool persistent_memory;
    struct core_vector output_blocks;
};

extern struct thorium_script biosal_assembly_arc_classifier_script;

#endif
