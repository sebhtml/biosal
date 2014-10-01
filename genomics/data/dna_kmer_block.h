
#ifndef BIOSAL_DNA_KMER_BLOCK
#define BIOSAL_DNA_KMER_BLOCK

#include <core/structures/vector.h>

#include <genomics/data/dna_codec.h>

#include <core/system/memory_pool.h>

struct biosal_dna_kmer;

struct biosal_dna_kmer_block {

    int source_index;
    int kmer_length;

    struct core_vector kmers;
};

void biosal_dna_kmer_block_init(struct biosal_dna_kmer_block *self, int kmer_length,
                int source_index, int kmers);
void biosal_dna_kmer_block_init_empty(struct biosal_dna_kmer_block *self);

void biosal_dna_kmer_block_destroy(struct biosal_dna_kmer_block *self, struct core_memory_pool *memory);
void biosal_dna_kmer_block_add_kmer(struct biosal_dna_kmer_block *self, struct biosal_dna_kmer *kmer,
                struct core_memory_pool *memory, struct biosal_dna_codec *codec);

int biosal_dna_kmer_block_pack_size(struct biosal_dna_kmer_block *self, struct biosal_dna_codec *codec);
int biosal_dna_kmer_block_pack(struct biosal_dna_kmer_block *self, void *buffer, struct biosal_dna_codec *codec);
int biosal_dna_kmer_block_unpack(struct biosal_dna_kmer_block *self, void *buffer, struct core_memory_pool *memory,
                struct biosal_dna_codec *codec);

int biosal_dna_kmer_block_pack_unpack(struct biosal_dna_kmer_block *self, void *buffer,
                int operation, struct core_memory_pool *memory,
                struct biosal_dna_codec *codec);

int biosal_dna_kmer_block_source_index(struct biosal_dna_kmer_block *self);
struct core_vector *biosal_dna_kmer_block_kmers(struct biosal_dna_kmer_block *self);

int biosal_dna_kmer_block_size(struct biosal_dna_kmer_block *self);

#endif
