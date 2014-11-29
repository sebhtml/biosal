
#ifndef BIOSAL_UNITIG_VISITOR_H
#define BIOSAL_UNITIG_VISITOR_H

#include "unitig_heuristic.h"

#include <genomics/assembly/assembly_vertex.h>
#include <genomics/assembly/vertex_neighborhood.h>

#include <genomics/data/dna_codec.h>
#include <genomics/data/dna_kmer.h>

#include <core/system/timer.h>
#include <core/system/memory_pool.h>

#include <engine/thorium/actor.h>

#include <stdbool.h>

#define SCRIPT_UNITIG_VISITOR 0xeb3b39fd

/**
 * A visitor for unitigs.
 */
struct biosal_unitig_visitor {
    struct core_vector graph_stores;

    struct biosal_dna_codec codec;

    struct biosal_dna_kmer main_kmer;
    struct biosal_dna_kmer parent_kmer;
    struct biosal_dna_kmer child_kmer;
    struct biosal_vertex_neighborhood main_neighborhood;
    struct biosal_vertex_neighborhood parent_neighborhood;
    struct biosal_vertex_neighborhood child_neighborhood;
    int selected_parent;
    int selected_child;
    int selected_parent_child;
    int selected_child_parent;

    struct core_memory_pool memory_pool;

    int graph_store_index;
    int completed;
    int manager;

    int kmer_length;
    int step;
    int visited_vertices;
    int vertices_with_unitig_flag;

    struct biosal_unitig_heuristic heuristic;

    /*
     * Attributes for progress.
     */

    int last_visited_count;
    uint64_t last_second_count;
};

extern struct thorium_script biosal_unitig_visitor_script;

#endif
