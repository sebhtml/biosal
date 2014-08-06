
#ifndef BSAL_ASSEMBLY_ARC_H
#define BSAL_ASSEMBLY_ARC_H

#include <genomics/data/dna_kmer.h>

#define BSAL_ARC_TYPE_PARENT 0
#define BSAL_ARC_TYPE_CHILD 1

/*
 * An assembly arc.
 *
 * This is used for transportation
 */
struct bsal_assembly_arc {

    struct bsal_dna_kmer source;

    /*
     * The type is child or parent
     */
    int type;

    /*
     * The destination is A, T, C, or G
     */
    int destination;
};

void bsal_assembly_arc_init(struct bsal_assembly_arc *self, int type,
                struct bsal_dna_kmer *source,
                int destination,
                int kmer_length, struct bsal_memory_pool *memory,
                struct bsal_dna_codec *codec);
void bsal_assembly_arc_init_empty(struct bsal_assembly_arc *self);
void bsal_assembly_arc_destroy(struct bsal_assembly_arc *self, struct bsal_memory_pool *memory);

int bsal_assembly_arc_pack_size(struct bsal_assembly_arc *self, int kmer_length, struct bsal_dna_codec *codec);
int bsal_assembly_arc_pack(struct bsal_assembly_arc *self, void *buffer,
                int kmer_length, struct bsal_dna_codec *codec);
int bsal_assembly_arc_unpack(struct bsal_assembly_arc *self, void *buffer,
                int kmer_length, struct bsal_memory_pool *memory,
                struct bsal_dna_codec *codec);

int bsal_assembly_arc_pack_unpack(struct bsal_assembly_arc *self, int operation,
                void *buffer, int kmer_length, struct bsal_memory_pool *memory,
                struct bsal_dna_codec *codec);

int bsal_assembly_arc_equals(struct bsal_assembly_arc *self, struct bsal_assembly_arc *arc,
                int kmer_length, struct bsal_dna_codec *codec);

#endif
