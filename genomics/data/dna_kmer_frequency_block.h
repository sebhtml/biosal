
#ifndef BIOSAL_DNA_KMER_FREQUENCY_BLOCK
#define BIOSAL_DNA_KMER_FREQUENCY_BLOCK

#include <core/structures/map.h>

#include <genomics/data/dna_codec.h>

#include <core/system/memory_pool.h>

struct biosal_dna_kmer;

struct biosal_dna_kmer_frequency_block {

    int kmer_length;

    struct core_map kmers;
};

void biosal_dna_kmer_frequency_block_init(struct biosal_dna_kmer_frequency_block *self, int kmer_length,
                struct core_memory_pool *memory, struct biosal_dna_codec *codec,
                int estimated_kmer_count);

void biosal_dna_kmer_frequency_block_destroy(struct biosal_dna_kmer_frequency_block *self, struct core_memory_pool *memory);

void biosal_dna_kmer_frequency_block_add_kmer(struct biosal_dna_kmer_frequency_block *self, struct biosal_dna_kmer *kmer,
                struct core_memory_pool *memory, struct biosal_dna_codec *codec);

int biosal_dna_kmer_frequency_block_pack_size(struct biosal_dna_kmer_frequency_block *self, struct biosal_dna_codec *codec);
int biosal_dna_kmer_frequency_block_pack(struct biosal_dna_kmer_frequency_block *self, void *buffer, struct biosal_dna_codec *codec);
int biosal_dna_kmer_frequency_block_unpack(struct biosal_dna_kmer_frequency_block *self, void *buffer, struct core_memory_pool *memory,
                struct biosal_dna_codec *codec);

int biosal_dna_kmer_frequency_block_pack_unpack(struct biosal_dna_kmer_frequency_block *self, void *buffer,
                int operation, struct core_memory_pool *memory,
                struct biosal_dna_codec *codec);

struct core_map *biosal_dna_kmer_frequency_block_kmers(struct biosal_dna_kmer_frequency_block *self);
int biosal_dna_kmer_frequency_block_empty(struct biosal_dna_kmer_frequency_block *self);

#endif
