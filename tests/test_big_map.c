
#include <data/dna_kmer.h>
#include <structures/map.h>
#include <structures/map_iterator.h>
#include <system/memory.h>

#include "test.h"

#include <inttypes.h>

int main(int argc, char **argv)
{
    BEGIN_TESTS();

    {
        struct bsal_map big_map;
        int kmer_length = 43;
        struct bsal_dna_kmer kmer;
        int count;
        int run_test;
        int coverage;
        void *key;
        int key_length;
        int *bucket;
        int i;

        run_test = 1;
        count = 100000000;

        printf("STRESS TEST\n");

        bsal_dna_kmer_init_mock(&kmer, kmer_length);
        key_length = bsal_dna_kmer_pack_size(&kmer);
        bsal_dna_kmer_destroy(&kmer);

        bsal_map_init(&big_map, key_length, sizeof(coverage));

        key = bsal_malloc(key_length);

        i = 0;
        while (i < count && run_test) {

            bsal_dna_kmer_init_random(&kmer, kmer_length);
            bsal_dna_kmer_pack_store_key(&kmer, key);

            bucket = bsal_map_add(&big_map, key);
            coverage = 99;
            (*bucket) = coverage;

            bsal_dna_kmer_destroy(&kmer);

            if (i % 100000 == 0) {
                printf("ADD %d/%d %" PRIu64 "\n", i, count,
                                bsal_map_size(&big_map));
            }
            i++;
        }

        bsal_map_destroy(&big_map);
        bsal_free(key);
    }

    END_TESTS();

    return 0;
}
