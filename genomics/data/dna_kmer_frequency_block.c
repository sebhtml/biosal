
#include "dna_kmer_frequency_block.h"

#include "dna_kmer.h"

#include <core/system/packer.h>

#include <stdio.h>

void bsal_dna_kmer_frequency_block_init(struct bsal_dna_kmer_frequency_block *self, int kmer_length,
                struct bsal_memory_pool *memory, struct bsal_dna_codec *codec,
                int estimated_kmer_count)
{
    struct bsal_dna_kmer kmer;
    int key_size;

    bsal_dna_kmer_init_mock(&kmer, kmer_length, codec, memory);
    key_size = bsal_dna_kmer_pack_size(&kmer, kmer_length, codec);

    bsal_dna_kmer_destroy(&kmer, memory);

    bsal_map_init(&self->kmers, key_size, sizeof(int));

    self->kmer_length = kmer_length;
}

void bsal_dna_kmer_frequency_block_destroy(struct bsal_dna_kmer_frequency_block *self, struct bsal_memory_pool *memory)
{
    bsal_map_destroy(&self->kmers);
}

void bsal_dna_kmer_frequency_block_add_kmer(struct bsal_dna_kmer_frequency_block *self, struct bsal_dna_kmer *kmer,
                struct bsal_memory_pool *memory, struct bsal_dna_codec *codec)
{
    void *encoded_kmer;
    int size;
    int *bucket;

    size = bsal_dna_kmer_pack_size(kmer, self->kmer_length, codec);

    encoded_kmer = bsal_memory_pool_allocate(memory, size);

    bsal_dna_kmer_pack(kmer, encoded_kmer, self->kmer_length, codec);

    bucket = (int *)bsal_map_get(&self->kmers, encoded_kmer);

    if (bucket == NULL) {

        bucket = (int *)bsal_map_add(&self->kmers, encoded_kmer);
        (*bucket) = 0;
    }

    ++(*bucket);

    bsal_memory_pool_free(memory, encoded_kmer);
}

int bsal_dna_kmer_frequency_block_pack_size(struct bsal_dna_kmer_frequency_block *self, struct bsal_dna_codec *codec)
{

    return bsal_dna_kmer_frequency_block_pack_unpack(self, NULL, BSAL_PACKER_OPERATION_DRY_RUN,
                    NULL, codec);
}

int bsal_dna_kmer_frequency_block_pack(struct bsal_dna_kmer_frequency_block *self, void *buffer, struct bsal_dna_codec *codec)
{

    return bsal_dna_kmer_frequency_block_pack_unpack(self, buffer, BSAL_PACKER_OPERATION_PACK,
                    NULL, codec);
}

int bsal_dna_kmer_frequency_block_unpack(struct bsal_dna_kmer_frequency_block *self, void *buffer, struct bsal_memory_pool *memory,
                struct bsal_dna_codec *codec)
{

    return bsal_dna_kmer_frequency_block_pack_unpack(self, buffer, BSAL_PACKER_OPERATION_UNPACK,
                    memory, codec);
}

int bsal_dna_kmer_frequency_block_pack_unpack(struct bsal_dna_kmer_frequency_block *self, void *buffer,
                int operation, struct bsal_memory_pool *memory,
                struct bsal_dna_codec *codec)
{
    int bytes;
    struct bsal_packer packer;

    bsal_packer_init(&packer, operation, buffer);

    bytes = 0;

    bsal_packer_work(&packer, &self->kmer_length, sizeof(self->kmer_length));

    bytes += bsal_packer_worked_bytes(&packer);

    bytes += bsal_map_pack_unpack(&self->kmers, operation, ((char *) buffer) + bytes);

    bsal_packer_destroy(&packer);

#if 0
    printf("packed %d\n", bytes);
#endif

    return bytes;
}

struct bsal_map *bsal_dna_kmer_frequency_block_kmers(struct bsal_dna_kmer_frequency_block *self)
{
    return &self->kmers;
}
