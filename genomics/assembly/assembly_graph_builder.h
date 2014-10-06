
#ifndef _BIOSAL_ASSEMBLY_GRAPH_BUILDER_H_
#define _BIOSAL_ASSEMBLY_GRAPH_BUILDER_H_

#include <biosal.h>

#include "assembly_graph_summary.h"

#define SCRIPT_ASSEMBLY_GRAPH_BUILDER 0xc0b1a2b3

#define ACTION_ASSEMBLY_GRAPH_BUILDER_CONTROL_COMPLEXITY 0x00006439

/*
 * Message tags for the graph builder.
 */
#define ACTION_VERIFY_ARCS 0x000032be

/*
 * This actor builds an assembly graph.
 * It needs a list of sequence stores and returns basically a list of
 * graph stores.
 */
struct biosal_assembly_graph_builder {
    struct core_vector spawners;
    struct core_vector sequence_stores;
    struct core_timer timer;
    struct core_timer vertex_timer;
    struct core_timer arc_timer;

    int source;

    int manager_for_graph_stores;
    struct core_vector graph_stores;

    int manager_for_windows;
    struct core_vector sliding_windows;

    int manager_for_classifiers;
    struct core_vector block_classifiers;

    int manager_for_arc_kernels;
    struct core_vector arc_kernels;

    int manager_for_arc_classifiers;
    struct core_vector arc_classifiers;

    int configured_sliding_windows;
    int configured_block_classifiers;
    int configured_graph_stores;

    int coverage_distribution;

    int actors_with_kmer_length;

    int kmer_length;

    uint64_t total_kmer_count;
    int notified_windows;

    uint64_t actual_kmer_count;
    int synchronized_graph_stores;

    int completed_sliding_windows;

    int doing_arcs;

    int configured_actors_for_arcs;
    int completed_arc_kernels;
    int configured_sequence_stores;

    uint64_t expected_arc_count;

    /*
     * Summary data
     */

    struct biosal_assembly_graph_summary graph_summary;

    int ready_graph_store_count;
};

extern struct thorium_script biosal_assembly_graph_builder_script;

#endif
