
#include "dna_codec.h"

#include <structures/vector.h>

#include <helpers/vector_helper.h>
#include <system/memory.h>

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
 * use block encoder (faster)
 */
#define BSAL_DNA_CODEC_USE_TWO_BIT_BLOCK_ENCODER

/*
 * use block decoder (faster)
 */
#define BSAL_DNA_CODEC_USE_TWO_BIT_BLOCK_DECODER

/*
*/

void bsal_dna_codec_init(struct bsal_dna_codec *self)
{
    /* 4 * 2 = 8 bits = 1 byte
     */
    self->block_length = 4;

    self->use_two_bit_encoding = 0;

    bsal_map_init(&self->encoding_lookup_table, self->block_length, 1);
    bsal_map_init(&self->decoding_lookup_table, 1, self->block_length);

#ifdef BSAL_DNA_CODEC_USE_TWO_BIT_BLOCK_ENCODER
    bsal_dna_codec_generate_blocks(self);
#endif

#ifdef BSAL_DNA_CODEC_FORCE_TWO_BIT_ENCODING
    bsal_dna_codec_enable_two_bit_encoding(self);
#endif
}

void bsal_dna_codec_destroy(struct bsal_dna_codec *self)
{
    bsal_map_destroy(&self->encoding_lookup_table);
    bsal_map_destroy(&self->decoding_lookup_table);
    self->block_length = 0;
}

void bsal_dna_codec_generate_blocks(struct bsal_dna_codec *self)
{
    char *block;

    block = bsal_memory_allocate(self->block_length + 1);

    bsal_dna_codec_generate_block(self, -1, 'X', block);

    bsal_memory_free(block);
}

void bsal_dna_codec_generate_block(struct bsal_dna_codec *self, int position, char symbol,
                char *block)
{
    char buffer[10];
    void *bucket;

#ifdef BSAL_DNA_CODEC_DEBUG
    printf("DEBUG position %d symbol %c\n", position, symbol);
#endif

    if (position >= 0) {
        block[position] = symbol;
    }

    position++;

    if (position < self->block_length && symbol != '\0') {

        bsal_dna_codec_generate_block(self, position, 'A', block);
        bsal_dna_codec_generate_block(self, position, 'T', block);
        bsal_dna_codec_generate_block(self, position, 'G', block);
        bsal_dna_codec_generate_block(self, position, 'C', block);
        /*bsal_dna_codec_generate_block(self, position, '\0', block);*/
    } else {

        while (position < self->block_length + 1) {

            block[position] = '\0';
            position++;
        }

        bsal_dna_codec_encode_default(self, self->block_length, block, buffer);

#ifdef BSAL_DNA_CODEC_DEBUG
        printf("BLOCK %s (%d) value %d\n", block, self->block_length, (int)buffer[0]);
#endif

        bucket = bsal_map_add(&self->encoding_lookup_table, block);
        memcpy(bucket, buffer, 1);

        bucket = bsal_map_add(&self->decoding_lookup_table, buffer);
        memcpy(bucket, block, self->block_length);
    }

}

int bsal_dna_codec_encoded_length(struct bsal_dna_codec *self, int length_in_nucleotides)
{
    if (self->use_two_bit_encoding) {
        return bsal_dna_codec_encoded_length_default(self, length_in_nucleotides);
    }

    return length_in_nucleotides + 1;
}

int bsal_dna_codec_encoded_length_default(struct bsal_dna_codec *self, int length_in_nucleotides)
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

void bsal_dna_codec_encode(struct bsal_dna_codec *self,
                int length_in_nucleotides, char *dna_sequence, void *encoded_sequence)
{
    if (self->use_two_bit_encoding) {
        bsal_dna_codec_encode_with_blocks(self, length_in_nucleotides, dna_sequence, encoded_sequence);
    } else {
        strcpy(encoded_sequence, dna_sequence);
    }
}

void bsal_dna_codec_encode_with_blocks(struct bsal_dna_codec *self,
                int length_in_nucleotides, char *dna_sequence, void *encoded_sequence)
{

#if 0
    bsal_dna_codec_encode_default(length_in_nucleotides, dna_sequence, encoded_sequence);
#endif
    int position;
    uint8_t byte;
    char data[5];
    int remaining;
    char *block;
    int byte_position;

    byte_position = 0;
    position = 0;

#ifdef BSAL_DNA_CODEC_DEBUG
    printf("DEBUG encoding %s\n", dna_sequence);
#endif

    while (position < length_in_nucleotides) {


        remaining = length_in_nucleotides - position;
        block = dna_sequence + position;

#ifdef BSAL_DNA_CODEC_DEBUG
        printf("DEBUG position %d remaining %d block_length %d\n",
                        position, remaining, self->block_length);
#endif

        /* a buffer is required when less than block size
         * nucleotides remain
         */
        if (remaining < self->block_length) {

            /* A is 00, so it is the same as nothing
             */
            memset(data, 'A', self->block_length);
            memcpy(data, block, remaining);
            block = data;
        }

#ifdef BSAL_DNA_CODEC_DEBUG
        printf("DEBUG block %c%c%c%c\n", block[0], block[1], block[2], block[3]);
#endif

        byte = *(char *)bsal_map_get(&self->encoding_lookup_table, block);
        ((char *)encoded_sequence)[byte_position] = byte;

        byte_position++;
        position += self->block_length;
    }
}

void bsal_dna_codec_encode_default(struct bsal_dna_codec *codec,
                int length_in_nucleotides, char *dna_sequence, void *encoded_sequence)
{
    int i;
    int encoded_length;

    encoded_length = bsal_dna_codec_encoded_length(codec, length_in_nucleotides);

#ifdef BSAL_DNA_CODEC_DEBUG
    printf("DEBUG encoding %s %d nucleotides, encoded_length %d\n", dna_sequence, length_in_nucleotides,
                    encoded_length);
#endif

    memset(encoded_sequence, 0, encoded_length);

    i = 0;

    while (i < length_in_nucleotides) {

        bsal_dna_codec_set_nucleotide(encoded_sequence, i, dna_sequence[i]);

        i++;
    }
}

void bsal_dna_codec_decode(struct bsal_dna_codec *codec,
                int length_in_nucleotides, void *encoded_sequence, char *dna_sequence)
{
    if (codec->use_two_bit_encoding) {
        bsal_dna_codec_decode_with_blocks(codec, length_in_nucleotides, encoded_sequence, dna_sequence);
    } else {
        strcpy(dna_sequence, encoded_sequence);
    }
}

void bsal_dna_codec_decode_with_blocks(struct bsal_dna_codec *self,
                int length_in_nucleotides, void *encoded_sequence, char *dna_sequence)
{
    char byte;
    int nucleotide_position;
    int encoded_position;
    int remaining;
    void *encoded_block;
    int to_copy;

    encoded_position = 0;
    nucleotide_position = 0;

    while (nucleotide_position < length_in_nucleotides) {

        remaining = length_in_nucleotides - nucleotide_position;
        byte = ((char *)encoded_sequence)[encoded_position];
        encoded_block = bsal_map_get(&self->decoding_lookup_table, &byte);
        to_copy = self->block_length;

        if (to_copy > remaining) {
            to_copy = remaining;
        }
        memcpy(dna_sequence + nucleotide_position, encoded_block, to_copy);

        nucleotide_position += self->block_length;
        encoded_position++;
    }

    dna_sequence[length_in_nucleotides] = '\0';
}

void bsal_dna_codec_decode_default(struct bsal_dna_codec *self, int length_in_nucleotides, void *encoded_sequence, char *dna_sequence)
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

/* this would be diccult to do because the padding at the end is not
 * always a multiple of block_length
 */
void bsal_dna_codec_reverse_complement_in_place(struct bsal_dna_codec *self,
                int length_in_nucleotides, void *encoded_sequence)
{
}

void bsal_dna_codec_enable_two_bit_encoding(struct bsal_dna_codec *codec)
{
    codec->use_two_bit_encoding = 1;
}

void bsal_dna_codec_disable_two_bit_encoding(struct bsal_dna_codec *codec)
{
    codec->use_two_bit_encoding = 0;
}
