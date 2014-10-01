
#include "test.h"

#include <genomics/data/dna_sequence.h>
#include <genomics/data/dna_codec.h>
#include <core/system/memory.h>
#include <core/system/memory_pool.h>

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    BEGIN_TESTS();

    void *buffer;
    struct biosal_dna_sequence sequence;
    struct biosal_dna_sequence sequence2;
    struct biosal_dna_codec codec;
    int actual;
    int expected;
    int required;
    void *buffer_for_pack;
    struct biosal_memory_pool memory;

    biosal_memory_pool_init(&memory, 4194304, BIOSAL_MEMORY_POOL_NAME_OTHER);
    buffer = biosal_memory_allocate(101, -1);
    strcpy((char *)buffer, "TCCCGAGCGCAGGTAGGCCTCGGGATCGATGTCCGGGGTGTTGAGGATGTTGGACGTGTATTCGTGGTTGTACTGGGTCCAGTCCGCCACCGGGCGCCGC");
    biosal_dna_codec_init(&codec);

    biosal_dna_sequence_init(&sequence, buffer, &codec, &memory);

    actual = biosal_dna_sequence_length(&sequence);
    expected = 100;
/*
    printf("DEBUG actual %d expected %d\n", actual, expected);
*/
    TEST_INT_EQUALS(actual, expected);

    required = biosal_dna_sequence_pack_size(&sequence, &codec);

    TEST_INT_IS_GREATER_THAN(required, 0);

    buffer_for_pack = biosal_memory_allocate(required, -1);

    /*
    printf("DEBUG buffer %p size for pack/unpack has %d bytes\n",
                    buffer_for_pack, required);
                    */

    biosal_dna_sequence_pack(&sequence, buffer_for_pack, &codec);
    biosal_dna_sequence_unpack(&sequence2, buffer_for_pack, &memory, &codec);

    actual = biosal_dna_sequence_length(&sequence2);

    /*
    printf("DEBUG actual %d expected %d\n", actual, expected);
    */
    TEST_INT_EQUALS(actual, expected);

    biosal_dna_sequence_destroy(&sequence, &memory);
    biosal_dna_sequence_destroy(&sequence2, &memory);

    biosal_dna_codec_destroy(&codec);
    biosal_memory_pool_destroy(&memory);
    END_TESTS();

    biosal_memory_free(buffer, -1);
    biosal_memory_free(buffer_for_pack, -1);

    return 0;
}

