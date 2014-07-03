
#include "test.h"

#include <genomics/helpers/dna_helper.h>
#include <genomics/data/dna_codec.h>

#include <core/system/memory.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    BEGIN_TESTS();

    struct bsal_dna_codec codec;
    void *encoded_sequence;
    int encoded_length;
    int sequence_length;
    /*
    char dna[] = "AAAATGAAAAAAATTAAAA";
    */
    char dna[] = "CACCCAGGGAGAGGGAGGACGCACCGAAAGAGAAA";
    char *sequence2;
    char *expected_sequence;

    bsal_dna_codec_init(&codec);
    sequence_length = strlen(dna);

    encoded_length = bsal_dna_codec_encoded_length(&codec, sequence_length);

    /*TEST_INT_IS_LOWER_THAN(encoded_length, sequence_length);*/
    TEST_INT_IS_GREATER_THAN(encoded_length, 0);
    TEST_INT_IS_GREATER_THAN(sequence_length, 0);

    encoded_sequence = bsal_memory_allocate(encoded_length);

    bsal_dna_codec_encode(&codec, sequence_length, dna, encoded_sequence);

    sequence2 = bsal_memory_allocate(sequence_length + 1);
    expected_sequence = bsal_memory_allocate(sequence_length + 1);

    TEST_POINTER_NOT_EQUALS(sequence2, NULL);

    bsal_dna_codec_decode(&codec, sequence_length, encoded_sequence, sequence2);

    /*
    printf("%s and\n%s\n", dna, sequence2);
    */

    TEST_BOOLEAN_EQUALS(strcmp(dna, sequence2) == 0, 1);

    /* test the codec reverse-complement feature
     */
    strcpy(expected_sequence, sequence2);
    bsal_dna_helper_reverse_complement_in_place(expected_sequence);

    bsal_dna_codec_reverse_complement_in_place(&codec, sequence_length, encoded_sequence);

    bsal_dna_codec_decode(&codec, sequence_length, encoded_sequence, sequence2);

#if 0
    if (strcmp(sequence2, expected_sequence) != 0) {
        printf("Actual   %s\n", sequence2);
        printf("Expected %s\n", expected_sequence);
    }

    TEST_BOOLEAN_EQUALS(strcmp(sequence2, expected_sequence) == 0, 1);
#endif

    bsal_memory_free(encoded_sequence);
    bsal_memory_free(sequence2);
    bsal_memory_free(expected_sequence);

    bsal_dna_codec_destroy(&codec);
    END_TESTS();

    return 0;
}
