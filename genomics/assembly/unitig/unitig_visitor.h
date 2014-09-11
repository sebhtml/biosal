
#ifndef BSAL_UNITIG_VISITOR_H
#define BSAL_UNITIG_VISITOR_H

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

/*
 * A visitor for unitigs.
 */
struct bsal_unitig_visitor {
    struct bsal_vector graph_stores;

    struct bsal_dna_codec codec;

    struct bsal_dna_kmer main_kmer;
    struct bsal_vertex_neighborhood main_neighborhood;
    struct bsal_vertex_neighborhood parent_neighborhood;
    struct bsal_vertex_neighborhood child_neighborhood;
    int selected_parent;
    int selected_child;

    struct bsal_memory_pool memory_pool;

    int graph_store_index;
    int completed;
    int manager;

    int kmer_length;
    int step;
    int visited;

    struct bsal_unitig_heuristic heuristic;
};

extern struct thorium_script bsal_unitig_visitor_script;

void bsal_unitig_visitor_init(struct thorium_actor *self);
void bsal_unitig_visitor_destroy(struct thorium_actor *self);
void bsal_unitig_visitor_receive(struct thorium_actor *self, struct thorium_message *message);

void bsal_unitig_visitor_execute(struct thorium_actor *self);

#endif
