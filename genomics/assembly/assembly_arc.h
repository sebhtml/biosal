
#ifndef BIOSAL_ASSEMBLY_ARC_H
#define BIOSAL_ASSEMBLY_ARC_H

#include <genomics/data/dna_kmer.h>

#define BIOSAL_ARC_TYPE_PARENT 0
#define BIOSAL_ARC_TYPE_CHILD 1
#define BIOSAL_ARC_TYPE_ANY 8

/*
 * An assembly arc.
 *
 * This is used for transportation
 */
struct biosal_assembly_arc {

    struct biosal_dna_kmer source;

    /*
     * The type is child or parent
     */
    int type;

    /*
     * The destination is A, T, C, or G
     */
    int destination;
};

void biosal_assembly_arc_init(struct biosal_assembly_arc *self, int type,
                struct biosal_dna_kmer *source,
                int destination,
                int kmer_length, struct biosal_memory_pool *memory,
                struct biosal_dna_codec *codec);
void biosal_assembly_arc_init_copy(struct biosal_assembly_arc *self,
                struct biosal_assembly_arc *arc,
                int kmer_length, struct biosal_memory_pool *memory,
                struct biosal_dna_codec *codec);
void biosal_assembly_arc_init_mock(struct biosal_assembly_arc *self,
                int kmer_length, struct biosal_memory_pool *memory,
                struct biosal_dna_codec *codec);

void biosal_assembly_arc_init_empty(struct biosal_assembly_arc *self);
void biosal_assembly_arc_destroy(struct biosal_assembly_arc *self, struct biosal_memory_pool *memory);

int biosal_assembly_arc_pack_size(struct biosal_assembly_arc *self, int kmer_length, struct biosal_dna_codec *codec);
int biosal_assembly_arc_pack(struct biosal_assembly_arc *self, void *buffer,
                int kmer_length, struct biosal_dna_codec *codec);
int biosal_assembly_arc_unpack(struct biosal_assembly_arc *self, void *buffer,
                int kmer_length, struct biosal_memory_pool *memory,
                struct biosal_dna_codec *codec);

int biosal_assembly_arc_pack_unpack(struct biosal_assembly_arc *self, int operation,
                void *buffer, int kmer_length, struct biosal_memory_pool *memory,
                struct biosal_dna_codec *codec);

int biosal_assembly_arc_equals(struct biosal_assembly_arc *self, struct biosal_assembly_arc *arc,
                int kmer_length, struct biosal_dna_codec *codec);

struct biosal_dna_kmer *biosal_assembly_arc_source(struct biosal_assembly_arc *self);
int biosal_assembly_arc_type(struct biosal_assembly_arc *self);
int biosal_assembly_arc_destination(struct biosal_assembly_arc *self);

void biosal_assembly_arc_print(struct biosal_assembly_arc *self, int kmer_length, struct biosal_dna_codec *codec,
                struct biosal_memory_pool *pool);

#endif
