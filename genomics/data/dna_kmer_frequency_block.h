
#ifndef BSAL_DNA_KMER_FREQUENCY_BLOCK
#define BSAL_DNA_KMER_FREQUENCY_BLOCK

#include <core/structures/map.h>

#include <genomics/data/dna_codec.h>

#include <core/system/memory_pool.h>

struct bsal_dna_kmer;

struct bsal_dna_kmer_frequency_block {

    int kmer_length;

    struct bsal_map kmers;
};

void bsal_dna_kmer_frequency_block_init(struct bsal_dna_kmer_frequency_block *self, int kmer_length,
                struct bsal_memory_pool *memory, struct bsal_dna_codec *codec,
                int estimated_kmer_count);

void bsal_dna_kmer_frequency_block_destroy(struct bsal_dna_kmer_frequency_block *self, struct bsal_memory_pool *memory);

void bsal_dna_kmer_frequency_block_add_kmer(struct bsal_dna_kmer_frequency_block *self, struct bsal_dna_kmer *kmer,
                struct bsal_memory_pool *memory, struct bsal_dna_codec *codec);

int bsal_dna_kmer_frequency_block_pack_size(struct bsal_dna_kmer_frequency_block *self, struct bsal_dna_codec *codec);
int bsal_dna_kmer_frequency_block_pack(struct bsal_dna_kmer_frequency_block *self, void *buffer, struct bsal_dna_codec *codec);
int bsal_dna_kmer_frequency_block_unpack(struct bsal_dna_kmer_frequency_block *self, void *buffer, struct bsal_memory_pool *memory,
                struct bsal_dna_codec *codec);

int bsal_dna_kmer_frequency_block_pack_unpack(struct bsal_dna_kmer_frequency_block *self, void *buffer,
                int operation, struct bsal_memory_pool *memory,
                struct bsal_dna_codec *codec);

struct bsal_map *bsal_dna_kmer_frequency_block_kmers(struct bsal_dna_kmer_frequency_block *self);

#endif
