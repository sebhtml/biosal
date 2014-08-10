
#include "test.h"

#include <genomics/helpers/dna_helper.h>
#include <genomics/data/dna_codec.h>

#include <core/system/memory.h>
#include <core/system/debugger.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
#define USE_TWO_BIT_ENCODING
*/

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

#ifdef USE_TWO_BIT_ENCODING
    bsal_dna_codec_enable_two_bit_encoding(&codec);
#endif

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

    if (strcmp(sequence2, expected_sequence) != 0) {
        printf("Origin   %s\n", dna);
        printf("Actual   %s\n", sequence2);
        printf("Expected %s\n", expected_sequence);
    }

#if 0
#endif
    TEST_BOOLEAN_EQUALS(strcmp(sequence2, expected_sequence) == 0, 1);

    bsal_dna_codec_set_nucleotide(&codec, encoded_sequence, 3, 'T');
    bsal_dna_codec_set_nucleotide(&codec, encoded_sequence, 2, 'T');
    bsal_dna_codec_set_nucleotide(&codec, encoded_sequence, 1, 'T');
    bsal_dna_codec_set_nucleotide(&codec, encoded_sequence, 4, 'T');
    bsal_dna_codec_set_nucleotide(&codec, encoded_sequence, 3, 'A');

#ifdef USE_TWO_BIT_ENCODING
    TEST_BOOLEAN_EQUALS(bsal_dna_codec_get_nucleotide(&codec, encoded_sequence, 3) == 'A', 1);
#endif


    bsal_memory_free(encoded_sequence);
    bsal_memory_free(sequence2);
    bsal_memory_free(expected_sequence);

    bsal_dna_codec_destroy(&codec);

    {
        struct bsal_dna_codec codec;
        char sequence2[] = "CGCGATCTGTTGCTGGGCCTAACGTGGTA";
        char sequence[] = "TACCACGTTAGGCCCAGCAACAGATCGCG";
        int kmer_length;
        void *encoded_data;
        void *encoded_data2;
        int encoded_length;

        bsal_dna_codec_init(&codec);
        bsal_dna_codec_enable_two_bit_encoding(&codec);

        kmer_length = strlen(sequence);

        encoded_length = bsal_dna_codec_encoded_length(&codec, kmer_length);

        encoded_data = bsal_memory_allocate(encoded_length);
        encoded_data2 = bsal_memory_allocate(encoded_length);

        memset(encoded_data, 0xff, encoded_length);

        bsal_dna_codec_encode(&codec, kmer_length, sequence, encoded_data);
        bsal_dna_codec_encode(&codec, kmer_length, sequence2, encoded_data2);

#if 0
        printf("Seq= %s\n", sequence);
        bsal_debugger_examine(encoded_data, encoded_length);
#endif

#if 0
        printf("after rev comp\n");
#endif
        bsal_dna_codec_reverse_complement_in_place(&codec, kmer_length, encoded_data);

#if 0
        bsal_debugger_examine(encoded_data, encoded_length);

        printf("Seq2 %s\n", sequence2);
        bsal_debugger_examine(encoded_data2, encoded_length);
#endif

        TEST_INT_EQUALS(memcmp(encoded_data, encoded_data2, encoded_length), 0);

        bsal_memory_free(encoded_data);
        bsal_memory_free(encoded_data2);

        bsal_dna_codec_destroy(&codec);

    }

    END_TESTS();

    return 0;
}
