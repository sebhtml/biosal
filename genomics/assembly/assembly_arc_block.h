
#ifndef BSAL_ASSEMBLY_ARC_BLOCK_H
#define BSAL_ASSEMBLY_ARC_BLOCK_H

#include "assembly_arc.h"

#include <core/structures/vector.h>
#include <core/structures/set.h>

/*
 * Some forward declarations since we only
 * need pointers size at this point.
 *
 * Link: http://en.wikipedia.org/wiki/Forward_declaration
 */
struct bsal_dna_codec;
struct bsal_memory_pool;
struct bsal_assembly_arc;

/*
 * This is a block of arcs.
 */
struct bsal_assembly_arc_block {
    struct bsal_set set;
    struct bsal_vector arcs;
};

void bsal_assembly_arc_block_init(struct bsal_assembly_arc_block *self, struct bsal_memory_pool *pool,
                int kmer_length, struct bsal_dna_codec *codec);
void bsal_assembly_arc_block_reserve(struct bsal_assembly_arc_block *self, int size);
void bsal_assembly_arc_block_destroy(struct bsal_assembly_arc_block *self, struct bsal_memory_pool *pool);
void bsal_assembly_arc_block_add_arc(struct bsal_assembly_arc_block *self, struct bsal_assembly_arc *arc,
                int kmer_length, struct bsal_dna_codec *codec,
                struct bsal_memory_pool *pool);

int bsal_assembly_arc_block_pack_size(struct bsal_assembly_arc_block *self, int kmer_length, struct bsal_dna_codec *codec);
int bsal_assembly_arc_block_pack(struct bsal_assembly_arc_block *self, void *buffer,
                int kmer_length, struct bsal_dna_codec *codec);
int bsal_assembly_arc_block_unpack(struct bsal_assembly_arc_block *self, void *buffer,
                int kmer_length, struct bsal_dna_codec *codec,
                struct bsal_memory_pool *pool);
int bsal_assembly_arc_block_pack_unpack(struct bsal_assembly_arc_block *self, int operation,
                void *buffer, int kmer_length, struct bsal_dna_codec *codec,
                struct bsal_memory_pool *pool);

struct bsal_vector *bsal_assembly_arc_block_get_arcs(struct bsal_assembly_arc_block *self);

#endif
