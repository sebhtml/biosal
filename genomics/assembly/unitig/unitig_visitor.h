
#ifndef BSAL_UNITIG_VISITOR_H
#define BSAL_UNITIG_VISITOR_H

#include <genomics/data/dna_codec.h>
#include <genomics/data/dna_kmer.h>

#include <genomics/assembly/assembly_vertex.h>

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
    struct bsal_assembly_vertex main_vertex;

    struct bsal_memory_pool memory_pool;

    int graph_store_index;
    int completed;
    int manager;

    int kmer_length;
    int step;
    int visited;
};

extern struct thorium_script bsal_unitig_visitor_script;

void bsal_unitig_visitor_init(struct thorium_actor *self);
void bsal_unitig_visitor_destroy(struct thorium_actor *self);
void bsal_unitig_visitor_receive(struct thorium_actor *self, struct thorium_message *message);

void bsal_unitig_visitor_fetch_remote_memory(struct thorium_actor *self, struct bsal_dna_kmer *kmer);
void bsal_unitig_visitor_do_something(struct thorium_actor *self);

#endif
