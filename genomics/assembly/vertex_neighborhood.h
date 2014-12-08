
#ifndef BIOSAL_VERTEX_NEIGHBORHOOD_H
#define BIOSAL_VERTEX_NEIGHBORHOOD_H

#include <genomics/data/dna_kmer.h>

#include <genomics/assembly/assembly_vertex.h>

#include <core/structures/vector.h>

#include <core/helpers/bitmap.h>

#include <stdint.h>

struct core_memory_pool;
struct biosal_dna_codec;
struct thorium_actor;
struct thorium_message;

#define BIOSAL_VERTEX_NEIGHBORHOOD_FLAG_SET_VISITOR_FLAG CORE_BITMAP_MAKE_FLAG(0)

/*
 * A vertex neighborhood.
 */
struct biosal_vertex_neighborhood {

    struct core_memory_pool *memory;
    struct biosal_dna_codec *codec;

    int fetch_parents;
    int fetch_children;

    struct biosal_dna_kmer main_kmer;
    struct biosal_assembly_vertex main_vertex;

    struct core_vector parent_vertices;
    struct core_vector child_vertices;

    struct core_vector *graph_stores;

    int kmer_length;
    int step;
    struct thorium_actor *actor;

    uint32_t flags;
};

void biosal_vertex_neighborhood_init(struct biosal_vertex_neighborhood *self,
               struct biosal_dna_kmer *kmer,
                int arcs, struct core_vector *graph_stores, int kmer_length,
                struct core_memory_pool *memory, struct biosal_dna_codec *codec,
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
