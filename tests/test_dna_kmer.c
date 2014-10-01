
#include "test.h"

#include <genomics/data/dna_kmer.h>
#include <genomics/data/dna_codec.h>

#include <core/system/memory_pool.h>

#include <string.h>

int main(int argc, char **argv)
{
    struct biosal_dna_kmer kmer;
    struct biosal_dna_kmer kmer2;
    void *buffer;
    int count;
    struct biosal_dna_codec codec;
    struct biosal_memory_pool pool;
    int kmer_length;
    char sequence[] = "ATGATCTGCAGTACTGAC";

    BEGIN_TESTS();

    kmer_length = strlen(sequence);
    biosal_dna_codec_init(&codec);
    biosal_dna_codec_enable_two_bit_encoding(&codec);

    biosal_memory_pool_init(&pool, 1000000, BIOSAL_MEMORY_POOL_NAME_OTHER);

    biosal_dna_kmer_init(&kmer, sequence, &codec, &pool);

    count = biosal_dna_kmer_pack_size(&kmer, kmer_length, &codec);

    TEST_INT_IS_GREATER_THAN(count, 0);

    buffer = biosal_memory_allocate(count, -1);

    biosal_dna_kmer_pack(&kmer, buffer, kmer_length, &codec);

    biosal_dna_kmer_init_empty(&kmer2);

    /* unpack and test
     */

    biosal_dna_kmer_unpack(&kmer2, buffer, kmer_length, &pool, &codec);

    TEST_BOOLEAN_EQUALS(biosal_dna_kmer_equals(&kmer, &kmer, kmer_length, &codec), 1);
    TEST_BOOLEAN_EQUALS(biosal_dna_kmer_equals(&kmer2, &kmer2, kmer_length, &codec), 1);
    TEST_BOOLEAN_EQUALS(biosal_dna_kmer_equals(&kmer, &kmer2, kmer_length, &codec), 1);
    TEST_BOOLEAN_EQUALS(biosal_dna_kmer_equals(&kmer2, &kmer, kmer_length, &codec), 1);

    biosal_dna_kmer_reverse_complement_self(&kmer2, kmer_length, &codec, &pool);
    biosal_dna_kmer_reverse_complement_self(&kmer2, kmer_length, &codec, &pool);

    TEST_BOOLEAN_EQUALS(biosal_dna_kmer_equals(&kmer, &kmer2, kmer_length, &codec), 1);

    biosal_dna_kmer_destroy(&kmer, &pool);
    biosal_dna_kmer_destroy(&kmer2, &pool);

    biosal_memory_pool_destroy(&pool);
    biosal_dna_codec_destroy(&codec);

    biosal_memory_free(buffer, -1);

    END_TESTS();

    return 0;
}


