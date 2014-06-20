
#include "dna_codec.h"

#include <string.h>
#include <stdio.h>

#define BITS_PER_NUCLEOTIDE 2
#define BITS_PER_BYTE 8

#define BSAL_NUCLEOTIDE_CODE_A 0 /* ~00 == 11 */
#define BSAL_NUCLEOTIDE_CODE_C 1 /* ~01 == 10 */
#define BSAL_NUCLEOTIDE_CODE_G 2 /* ~10 == 01 */
#define BSAL_NUCLEOTIDE_CODE_T 3 /* ~11 == 00 */

#define BSAL_NUCLEOTIDE_SYMBOL_A 'A'
#define BSAL_NUCLEOTIDE_SYMBOL_C 'C'
#define BSAL_NUCLEOTIDE_SYMBOL_G 'G'
#define BSAL_NUCLEOTIDE_SYMBOL_T 'T'

#include <stdint.h>

/*
 * enable 2-bit encoding
 */
#define BSAL_DNA_CODEC_USE_TWO_BIT_ENCODING_DEFAULT

/*
*/

int bsal_dna_codec_encoded_length(int length_in_nucleotides)
{
#ifdef BSAL_DNA_CODEC_USE_TWO_BIT_ENCODING_DEFAULT
    return bsal_dna_codec_encoded_length_default(length_in_nucleotides);
#else
    return length_in_nucleotides + 1;
#endif
}

int bsal_dna_codec_encoded_length_default(int length_in_nucleotides)
{
    int bits;
    int bytes;

    bits = length_in_nucleotides * BITS_PER_NUCLEOTIDE;

    /* padding
     */
    bits += (BITS_PER_BYTE - (bits % BITS_PER_BYTE));

    bytes = bits / BITS_PER_BYTE;

    return bytes;
}

void bsal_dna_codec_encode(int length_in_nucleotides, char *dna_sequence, void *encoded_sequence)
{
#ifdef BSAL_DNA_CODEC_USE_TWO_BIT_ENCODING_DEFAULT
    bsal_dna_codec_encode_default(length_in_nucleotides, dna_sequence, encoded_sequence);
#else
    strcpy(encoded_sequence, dna_sequence);
#endif
}

void bsal_dna_codec_encode_default(int length_in_nucleotides, char *dna_sequence, void *encoded_sequence)
{
    int i;
    int encoded_length;

    encoded_length = bsal_dna_codec_encoded_length(length_in_nucleotides);

    memset(encoded_sequence, 0, encoded_length);

    i = 0;

    while (i < length_in_nucleotides) {

        bsal_dna_codec_set_nucleotide(encoded_sequence, i, dna_sequence[i]);

        i++;
    }
}

void bsal_dna_codec_decode(int length_in_nucleotides, void *encoded_sequence, char *dna_sequence)
{
#ifdef BSAL_DNA_CODEC_USE_TWO_BIT_ENCODING_DEFAULT
    bsal_dna_codec_decode_default(length_in_nucleotides, encoded_sequence, dna_sequence);
#else
    strcpy(dna_sequence, encoded_sequence);
#endif
}

void bsal_dna_codec_decode_default(int length_in_nucleotides, void *encoded_sequence, char *dna_sequence)
{
    int i;

    i = 0;

    while (i < length_in_nucleotides) {

        dna_sequence[i] = bsal_dna_codec_get_nucleotide(encoded_sequence, i);

        i++;
    }

    dna_sequence[length_in_nucleotides] = '\0';
}

void bsal_dna_codec_set_nucleotide(void *encoded_sequence, int index, char nucleotide)
{
    int bit_index;
    int byte_index;
    int bit_index_in_byte;
    uint64_t old_byte_value;
    uint64_t mask;
    uint64_t new_byte_value;

    bit_index = index * BITS_PER_NUCLEOTIDE;
    byte_index = bit_index / BITS_PER_BYTE;
    bit_index_in_byte = bit_index % BITS_PER_BYTE;

#ifdef BSAL_DNA_CODEC_DEBUG
    printf("index %d nucleotide %c bit_index %d byte_index %d bit_index_in_byte %d\n",
                        index, nucleotide, bit_index, byte_index, bit_index_in_byte);

    if (nucleotide == BSAL_NUCLEOTIDE_SYMBOL_C) {
    }
#endif

    old_byte_value = ((uint8_t*)encoded_sequence)[byte_index];

    mask = bsal_dna_codec_get_code(nucleotide);

#ifdef BSAL_DNA_CODEC_DEBUG
    if (nucleotide == BSAL_NUCLEOTIDE_SYMBOL_C) {
        printf("DEBUG code is %d\n", (int)mask);
    }
#endif

    mask <<= bit_index_in_byte;

    new_byte_value = old_byte_value | mask;

#ifdef BSAL_DNA_CODEC_DEBUG
    printf("old: %d new: %d\n", (int)old_byte_value, (int)new_byte_value);

    if (nucleotide == BSAL_NUCLEOTIDE_SYMBOL_C) {

    }
#endif

    ((uint8_t *)encoded_sequence)[byte_index] = new_byte_value;
}

uint64_t bsal_dna_codec_get_code(char nucleotide)
{
    switch (nucleotide) {

        case BSAL_NUCLEOTIDE_SYMBOL_A:
            return BSAL_NUCLEOTIDE_CODE_A;
        case BSAL_NUCLEOTIDE_SYMBOL_T:
            return BSAL_NUCLEOTIDE_CODE_T;
        case BSAL_NUCLEOTIDE_SYMBOL_C:
            return BSAL_NUCLEOTIDE_CODE_C;
        case BSAL_NUCLEOTIDE_SYMBOL_G:
            return BSAL_NUCLEOTIDE_CODE_G;
    }

    return BSAL_NUCLEOTIDE_CODE_A;
}

char bsal_dna_codec_get_nucleotide(void *encoded_sequence, int index)
{
    int bit_index;
    int byte_index;
    int bit_index_in_byte;
    uint64_t byte_value;
    uint64_t code;

    bit_index = index * BITS_PER_NUCLEOTIDE;
    byte_index = bit_index / BITS_PER_BYTE;
    bit_index_in_byte = bit_index % BITS_PER_BYTE;

    byte_value = ((uint8_t *)encoded_sequence)[byte_index];

    code = (byte_value << (8 * BITS_PER_BYTE - BITS_PER_NUCLEOTIDE - bit_index_in_byte)) >> (8 * BITS_PER_BYTE - BITS_PER_NUCLEOTIDE);

#ifdef BSAL_DNA_CODEC_DEBUG
    printf("code %d\n", (int)code);
#endif

    return bsal_dna_codec_get_nucleotide_from_code(code);
}

char bsal_dna_codec_get_nucleotide_from_code(uint64_t code)
{
    switch (code) {
        case BSAL_NUCLEOTIDE_CODE_A:
            return BSAL_NUCLEOTIDE_SYMBOL_A;
        case BSAL_NUCLEOTIDE_CODE_T:
            return BSAL_NUCLEOTIDE_SYMBOL_T;
        case BSAL_NUCLEOTIDE_CODE_C:
            return BSAL_NUCLEOTIDE_SYMBOL_C;
        case BSAL_NUCLEOTIDE_CODE_G:
            return BSAL_NUCLEOTIDE_SYMBOL_G;
    }

    return BSAL_NUCLEOTIDE_SYMBOL_A;
}
