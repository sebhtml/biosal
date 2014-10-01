
#include <genomics/data/dna_kmer.h>
#include <genomics/data/dna_codec.h>
#include <core/structures/map.h>
#include <core/structures/map_iterator.h>
#include <core/system/memory.h>
#include <core/system/memory_pool.h>

#include "test.h"

#include <inttypes.h>

int main(int argc, char **argv)
{
    BEGIN_TESTS();

    {
        struct core_map big_map;
        int kmer_length = 43;
        struct biosal_dna_kmer kmer;
        int count;
        int run_test;
        int coverage;
        void *key;
        int key_length;
        int *bucket;
        int i;
        struct biosal_dna_codec codec;
        struct core_memory_pool memory;

        core_memory_pool_init(&memory, 1048576, CORE_MEMORY_POOL_NAME_OTHER);
        biosal_dna_codec_init(&codec);

        run_test = 1;
        count = 100000000;

        printf("STRESS TEST\n");

        biosal_dna_kmer_init_mock(&kmer, kmer_length, &codec, &memory);
        key_length = biosal_dna_kmer_pack_size(&kmer, kmer_length, &codec);
        biosal_dna_kmer_destroy(&kmer, &memory);

        core_map_init(&big_map, key_length, sizeof(coverage));

        key = core_memory_allocate(key_length, -1);

        i = 0;
        while (i < count && run_test) {

            biosal_dna_kmer_init_random(&kmer, kmer_length, &codec, &memory);
            biosal_dna_kmer_pack_store_key(&kmer, key, kmer_length, &codec, &memory);

            bucket = core_map_add(&big_map, key);
            coverage = 99;
            (*bucket) = coverage;

            biosal_dna_kmer_destroy(&kmer, &memory);

            if (i % 100000 == 0) {
                printf("ADD %d/%d %" PRIu64 "\n", i, count,
                                core_map_size(&big_map));
            }
            i++;
        }

        core_map_destroy(&big_map);
        core_memory_free(key, -1);
        biosal_dna_codec_destroy(&codec);
        core_memory_pool_destroy(&memory);
    }

    END_TESTS();

    return 0;
}
