
#ifndef BSAL_VERTEX_NEIGHBORHOOD_H
#define BSAL_VERTEX_NEIGHBORHOOD_H

#include <genomics/data/dna_kmer.h>

#include <genomics/assembly/assembly_vertex.h>

struct bsal_memory_pool;
struct bsal_dna_codec;
struct thorium_actor;
struct thorium_message;

/*
 * A vertex neighborhood.
 */
struct bsal_vertex_neighborhood {

    struct bsal_memory_pool *memory;
    struct bsal_dna_codec *codec;

    int fetch_parents;
    int fetch_children;

    struct bsal_dna_kmer main_kmer;
    struct bsal_assembly_vertex main_vertex;

    struct bsal_vector parent_vertices;
    struct bsal_vector child_vertices;

    struct bsal_vector *graph_stores;

    int kmer_length;
    int step;
    struct thorium_actor *actor;
};

void bsal_vertex_neighborhood_init(struct bsal_vertex_neighborhood *self,
               struct bsal_dna_kmer *kmer,
                int arcs, struct bsal_vector *graph_stores, int kmer_length,
                struct bsal_memory_pool *memory, struct bsal_dna_codec *codec,
                struct thorium_actor *actor);

void bsal_vertex_neighborhood_init_empty(struct bsal_vertex_neighborhood *self);

void bsal_vertex_neighborhood_destroy(struct bsal_vertex_neighborhood *self);

/*
 * Returns 1 if data is ready, 0 otherwise.
 */
int bsal_vertex_neighborhood_receive(struct bsal_vertex_neighborhood *self, struct thorium_message *message);

void bsal_vertex_neighborhood_receive_remote_memory(struct bsal_vertex_neighborhood *self, struct bsal_dna_kmer *kmer);

int bsal_vertex_neighborhood_do_something(struct bsal_vertex_neighborhood *self);

#endif
