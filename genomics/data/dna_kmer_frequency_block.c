
#include "dna_kmer_frequency_block.h"

#include "dna_kmer.h"

#include <core/system/packer.h>

#include <stdio.h>

void biosal_dna_kmer_frequency_block_init(struct biosal_dna_kmer_frequency_block *self, int kmer_length,
                struct core_memory_pool *memory, struct biosal_dna_codec *codec,
                int estimated_kmer_count)
{
    struct biosal_dna_kmer kmer;
    int key_size;

    biosal_dna_kmer_init_mock(&kmer, kmer_length, codec, memory);
    key_size = biosal_dna_kmer_pack_size(&kmer, kmer_length, codec);

    biosal_dna_kmer_destroy(&kmer, memory);

    core_map_init(&self->kmers, key_size, sizeof(int));
    core_map_set_memory_pool(&self->kmers, memory);

    self->kmer_length = kmer_length;
}

void biosal_dna_kmer_frequency_block_destroy(struct biosal_dna_kmer_frequency_block *self, struct core_memory_pool *memory)
{
    core_map_destroy(&self->kmers);
}

void biosal_dna_kmer_frequency_block_add_kmer(struct biosal_dna_kmer_frequency_block *self, struct biosal_dna_kmer *kmer,
                struct core_memory_pool *memory, struct biosal_dna_codec *codec)
{
    void *encoded_kmer;
    int size;
    int *bucket;

    size = biosal_dna_kmer_pack_size(kmer, self->kmer_length, codec);

    encoded_kmer = core_memory_pool_allocate(memory, size);

    biosal_dna_kmer_pack(kmer, encoded_kmer, self->kmer_length, codec);

    bucket = (int *)core_map_get(&self->kmers, encoded_kmer);

    if (bucket == NULL) {

        bucket = (int *)core_map_add(&self->kmers, encoded_kmer);
        (*bucket) = 0;
    }

    ++(*bucket);

    core_memory_pool_free(memory, encoded_kmer);
}

int biosal_dna_kmer_frequency_block_pack_size(struct biosal_dna_kmer_frequency_block *self, struct biosal_dna_codec *codec)
{

    return biosal_dna_kmer_frequency_block_pack_unpack(self, NULL, CORE_PACKER_OPERATION_PACK_SIZE,
                    NULL, codec);
}

int biosal_dna_kmer_frequency_block_pack(struct biosal_dna_kmer_frequency_block *self, void *buffer, struct biosal_dna_codec *codec)
{

    return biosal_dna_kmer_frequency_block_pack_unpack(self, buffer, CORE_PACKER_OPERATION_PACK,
                    NULL, codec);
}

int biosal_dna_kmer_frequency_block_unpack(struct biosal_dna_kmer_frequency_block *self, void *buffer, struct core_memory_pool *memory,
                struct biosal_dna_codec *codec)
{

    return biosal_dna_kmer_frequency_block_pack_unpack(self, buffer, CORE_PACKER_OPERATION_UNPACK,
                    memory, codec);
}

int biosal_dna_kmer_frequency_block_pack_unpack(struct biosal_dna_kmer_frequency_block *self, void *buffer,
                int operation, struct core_memory_pool *memory,
                struct biosal_dna_codec *codec)
{
    int bytes;
    struct core_packer packer;

    core_packer_init(&packer, operation, buffer);

    bytes = 0;

    core_packer_process(&packer, &self->kmer_length, sizeof(self->kmer_length));

    bytes += core_packer_get_byte_count(&packer);

    bytes += core_map_pack_unpack(&self->kmers, operation, ((char *) buffer) + bytes);

    core_packer_destroy(&packer);

#if 0
    printf("packed %d\n", bytes);
#endif

    return bytes;
}

struct core_map *biosal_dna_kmer_frequency_block_kmers(struct biosal_dna_kmer_frequency_block *self)
{
    return &self->kmers;
}
