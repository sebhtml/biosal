
#ifndef BSAL_DNA_KMER_BLOCK
#define BSAL_DNA_KMER_BLOCK

#include <core/structures/vector.h>

#include <genomics/data/dna_codec.h>

#include <core/system/memory_pool.h>

struct bsal_dna_kmer;

struct bsal_dna_kmer_block {

    int source_index;
    int kmer_length;

    struct bsal_vector kmers;
};

void bsal_dna_kmer_block_init(struct bsal_dna_kmer_block *self, int kmer_length,
                int source_index, int kmers);
void bsal_dna_kmer_block_init_empty(struct bsal_dna_kmer_block *self);

void bsal_dna_kmer_block_destroy(struct bsal_dna_kmer_block *self, struct bsal_memory_pool *memory);
void bsal_dna_kmer_block_add_kmer(struct bsal_dna_kmer_block *self, struct bsal_dna_kmer *kmer,
                struct bsal_memory_pool *memory, struct bsal_dna_codec *codec);

int bsal_dna_kmer_block_pack_size(struct bsal_dna_kmer_block *self, struct bsal_dna_codec *codec);
int bsal_dna_kmer_block_pack(struct bsal_dna_kmer_block *self, void *buffer, struct bsal_dna_codec *codec);
int bsal_dna_kmer_block_unpack(struct bsal_dna_kmer_block *self, void *buffer, struct bsal_memory_pool *memory,
                struct bsal_dna_codec *codec);

int bsal_dna_kmer_block_pack_unpack(struct bsal_dna_kmer_block *self, void *buffer,
                int operation, struct bsal_memory_pool *memory,
                struct bsal_dna_codec *codec);

int bsal_dna_kmer_block_source_index(struct bsal_dna_kmer_block *self);
struct bsal_vector *bsal_dna_kmer_block_kmers(struct bsal_dna_kmer_block *self);

int bsal_dna_kmer_block_size(struct bsal_dna_kmer_block *self);

#endif
