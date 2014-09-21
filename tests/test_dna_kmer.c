
#include "test.h"

#include <genomics/data/dna_kmer.h>
#include <genomics/data/dna_codec.h>

#include <core/system/memory_pool.h>

#include <string.h>

int main(int argc, char **argv)
{
    struct bsal_dna_kmer kmer;
    struct bsal_dna_kmer kmer2;
    void *buffer;
    int count;
    struct bsal_dna_codec codec;
    struct bsal_memory_pool pool;
    int kmer_length;
    char sequence[] = "ATGATCTGCAGTACTGAC";

    BEGIN_TESTS();

    kmer_length = strlen(sequence);
    bsal_dna_codec_init(&codec);
    bsal_dna_codec_enable_two_bit_encoding(&codec);

    bsal_memory_pool_init(&pool, 1000000, BSAL_MEMORY_POOL_NAME_OTHER);

    bsal_dna_kmer_init(&kmer, sequence, &codec, &pool);

    count = bsal_dna_kmer_pack_size(&kmer, kmer_length, &codec);

    TEST_INT_IS_GREATER_THAN(count, 0);

    buffer = bsal_memory_allocate(count);

    bsal_dna_kmer_pack(&kmer, buffer, kmer_length, &codec);

    bsal_dna_kmer_init_empty(&kmer2);

    /* unpack and test
     */

    bsal_dna_kmer_unpack(&kmer2, buffer, kmer_length, &pool, &codec);

    TEST_BOOLEAN_EQUALS(bsal_dna_kmer_equals(&kmer, &kmer, kmer_length, &codec), 1);
    TEST_BOOLEAN_EQUALS(bsal_dna_kmer_equals(&kmer2, &kmer2, kmer_length, &codec), 1);
    TEST_BOOLEAN_EQUALS(bsal_dna_kmer_equals(&kmer, &kmer2, kmer_length, &codec), 1);
    TEST_BOOLEAN_EQUALS(bsal_dna_kmer_equals(&kmer2, &kmer, kmer_length, &codec), 1);

    bsal_dna_kmer_reverse_complement_self(&kmer2, kmer_length, &codec, &pool);
    bsal_dna_kmer_reverse_complement_self(&kmer2, kmer_length, &codec, &pool);

    TEST_BOOLEAN_EQUALS(bsal_dna_kmer_equals(&kmer, &kmer2, kmer_length, &codec), 1);

    bsal_dna_kmer_destroy(&kmer, &pool);
    bsal_dna_kmer_destroy(&kmer2, &pool);

    bsal_memory_pool_destroy(&pool);
    bsal_dna_codec_destroy(&codec);

    bsal_memory_free(buffer);

    END_TESTS();

    return 0;
}


