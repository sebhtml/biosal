
#include "test.h"

#include <data/dna_codec.h>
#include <system/memory.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    BEGIN_TESTS();

    void *encoded_sequence;
    int encoded_length;
    int sequence_length;
    /*
    char dna[] = "AAAATGAAAAAAATTAAAA";
    */
    char dna[] = "CACCCAGGGAGAGGGAGGACGCACCGAAAGAGAAA";
    char *sequence2;

    sequence_length = strlen(dna);

    encoded_length = bsal_dna_codec_encoded_length(sequence_length);

    /*TEST_INT_IS_LOWER_THAN(encoded_length, sequence_length);*/
    TEST_INT_IS_GREATER_THAN(encoded_length, 0);
    TEST_INT_IS_GREATER_THAN(sequence_length, 0);

    encoded_sequence = bsal_malloc(encoded_length);

    bsal_dna_codec_encode(sequence_length, dna, encoded_sequence);

    sequence2 = bsal_malloc(sequence_length + 1);

    TEST_POINTER_NOT_EQUALS(sequence2, NULL);

    bsal_dna_codec_decode(sequence_length, encoded_sequence, sequence2);

    /*
    printf("%s and\n%s\n", dna, sequence2);
    */

    TEST_BOOLEAN_EQUALS(strcmp(dna, sequence2) == 0, 1);

    bsal_free(encoded_sequence);
    bsal_free(sequence2);

    END_TESTS();

    return 0;
}
