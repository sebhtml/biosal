
#ifndef BIOSAL_ASSEMBLY_ARC_BLOCK_H
#define BIOSAL_ASSEMBLY_ARC_BLOCK_H

#include "assembly_arc.h"

#include <core/structures/vector.h>
#include <core/structures/set.h>

/*
 * Some forward declarations since we only
 * need pointers size at this point.
 *
 * Link: http://en.wikipedia.org/wiki/Forward_declaration
 */
struct biosal_dna_codec;
struct core_memory_pool;
struct biosal_assembly_arc;

/*
 * This is a block of arcs.
 */
struct biosal_assembly_arc_block {
    struct core_set set;
    struct core_vector arcs;
    int enable_redundancy_check;
};

void biosal_assembly_arc_block_init(struct biosal_assembly_arc_block *self, struct core_memory_pool *pool,
                int kmer_length, struct biosal_dna_codec *codec);
void biosal_assembly_arc_block_reserve(struct biosal_assembly_arc_block *self, int size);
void biosal_assembly_arc_block_destroy(struct biosal_assembly_arc_block *self, struct core_memory_pool *pool);
void biosal_assembly_arc_block_add_arc(struct biosal_assembly_arc_block *self, int type,
                struct biosal_dna_kmer *kmer, int destination,
                int kmer_length, struct biosal_dna_codec *codec,
                struct core_memory_pool *pool);

int biosal_assembly_arc_block_pack_size(struct biosal_assembly_arc_block *self, int kmer_length, struct biosal_dna_codec *codec);
int biosal_assembly_arc_block_pack(struct biosal_assembly_arc_block *self, void *buffer,
                int kmer_length, struct biosal_dna_codec *codec);
int biosal_assembly_arc_block_unpack(struct biosal_assembly_arc_block *self, void *buffer,
                int kmer_length, struct biosal_dna_codec *codec,
                struct core_memory_pool *pool);
int biosal_assembly_arc_block_pack_unpack(struct biosal_assembly_arc_block *self, int operation,
                void *buffer, int kmer_length, struct biosal_dna_codec *codec,
                struct core_memory_pool *pool);

struct core_vector *biosal_assembly_arc_block_get_arcs(struct biosal_assembly_arc_block *self);

void biosal_assembly_arc_block_enable_redundancy_check(struct biosal_assembly_arc_block *self);

void biosal_assembly_arc_block_add_arc_copy(struct biosal_assembly_arc_block *self,
                struct biosal_assembly_arc *arc, int kmer_length, struct biosal_dna_codec *codec,
                struct core_memory_pool *pool);

#endif
