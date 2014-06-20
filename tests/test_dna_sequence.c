
#include "test.h"

#include <data/dna_sequence.h>
#include <data/dna_codec.h>
#include <system/memory.h>

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    BEGIN_TESTS();

    void *buffer;
    struct bsal_dna_sequence sequence;
    struct bsal_dna_sequence sequence2;
    struct bsal_dna_codec codec;
    int actual;
    int expected;
    int required;
    void *buffer_for_pack;

    buffer = bsal_malloc(101);
    strcpy((char *)buffer, "TCCCGAGCGCAGGTAGGCCTCGGGATCGATGTCCGGGGTGTTGAGGATGTTGGACGTGTATTCGTGGTTGTACTGGGTCCAGTCCGCCACCGGGCGCCGC");
    bsal_dna_codec_init(&codec);

    bsal_dna_sequence_init(&sequence, buffer, &codec);

    actual = bsal_dna_sequence_length(&sequence);
    expected = 100;
/*
    printf("DEBUG actual %d expected %d\n", actual, expected);
*/
    TEST_INT_EQUALS(actual, expected);

    required = bsal_dna_sequence_pack_size(&sequence);

    TEST_INT_IS_GREATER_THAN(required, 0);

    buffer_for_pack = bsal_malloc(required);

    /*
    printf("DEBUG buffer %p size for pack/unpack has %d bytes\n",
                    buffer_for_pack, required);
                    */

    bsal_dna_sequence_pack(&sequence, buffer_for_pack);
    bsal_dna_sequence_unpack(&sequence2, buffer_for_pack);

    actual = bsal_dna_sequence_length(&sequence2);

    /*
    printf("DEBUG actual %d expected %d\n", actual, expected);
    */
    TEST_INT_EQUALS(actual, expected);

    bsal_dna_sequence_destroy(&sequence);

    bsal_dna_codec_destroy(&codec);
    END_TESTS();

    return 0;
}

