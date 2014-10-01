
#ifndef BIOSAL_VERTEX_NEIGHBORHOOD_H
#define BIOSAL_VERTEX_NEIGHBORHOOD_H

#include <genomics/data/dna_kmer.h>

#include <genomics/assembly/assembly_vertex.h>

struct biosal_memory_pool;
struct biosal_dna_codec;
struct thorium_actor;
struct thorium_message;

/*
 * A vertex neighborhood.
 */
struct biosal_vertex_neighborhood {

    struct biosal_memory_pool *memory;
    struct biosal_dna_codec *codec;

    int fetch_parents;
    int fetch_children;

    struct biosal_dna_kmer main_kmer;
    struct biosal_assembly_vertex main_vertex;

    struct biosal_vector parent_vertices;
    struct biosal_vector child_vertices;

    struct biosal_vector *graph_stores;

    int kmer_length;
    int step;
    struct thorium_actor *actor;
};

void biosal_vertex_neighborhood_init(struct biosal_vertex_neighborhood *self,
               struct biosal_dna_kmer *kmer,
                int arcs, struct biosal_vector *graph_stores, int kmer_length,
                struct biosal_memory_pool *memory, struct biosal_dna_codec *codec,
                struct thorium_actor *actor);

void biosal_vertex_neighborhood_init_empty(struct biosal_vertex_neighborhood *self);

void biosal_vertex_neighborhood_destroy(struct biosal_vertex_neighborhood *self);

/*
 * Returns 1 if data is ready, 0 otherwise.
 */
int biosal_vertex_neighborhood_receive(struct biosal_vertex_neighborhood *self, struct thorium_message *message);

void biosal_vertex_neighborhood_receive_remote_memory(struct biosal_vertex_neighborhood *self, struct biosal_dna_kmer *kmer);

int biosal_vertex_neighborhood_execute(struct biosal_vertex_neighborhood *self);

struct biosal_assembly_vertex *biosal_vertex_neighborhood_vertex(struct biosal_vertex_neighborhood *self);
struct biosal_assembly_vertex *biosal_vertex_neighborhood_parent(struct biosal_vertex_neighborhood *self, int i);
struct biosal_assembly_vertex *biosal_vertex_neighborhood_child(struct biosal_vertex_neighborhood *self, int i);

#endif
