
#ifndef BSAL_DNA_CODEC_H
#define BSAL_DNA_CODEC_H

#include <stdint.h>

#include <core/structures/map.h>

#define BSAL_DNA_CODEC_HAS_REVERSE_COMPLEMENT_IMPLEMENTATION

/*
*/
#define BSAL_DNA_CODEC_USE_TWO_BIT_ENCODING_FOR_TRANSPORT
#define BSAL_DNA_CODEC_USE_TWO_BIT_ENCODING_FOR_STORAGE

#define BSAL_DNA_CODEC_MINIMUM_NODE_COUNT_FOR_TWO_BIT (2)

#define BSAL_NUCLEOTIDE_CODE_A 0 /* ~00 == 11 */
#define BSAL_NUCLEOTIDE_CODE_C 1 /* ~01 == 10 */
#define BSAL_NUCLEOTIDE_CODE_G 2 /* ~10 == 01 */
#define BSAL_NUCLEOTIDE_CODE_T 3 /* ~11 == 00 */

#define BSAL_NUCLEOTIDE_SYMBOL_A 'A'
#define BSAL_NUCLEOTIDE_SYMBOL_C 'C'
#define BSAL_NUCLEOTIDE_SYMBOL_G 'G'
#define BSAL_NUCLEOTIDE_SYMBOL_T 'T'


/*
 * A class to encode and decode DNA data.
 */
struct bsal_dna_codec {
    struct bsal_map encoding_lookup_table;
    struct bsal_map decoding_lookup_table;
    int block_length;

    int use_two_bit_encoding;
};

void bsal_dna_codec_init(struct bsal_dna_codec *self);
void bsal_dna_codec_destroy(struct bsal_dna_codec *self);

int bsal_dna_codec_encoded_length(struct bsal_dna_codec *self, int length_in_nucleotides);
void bsal_dna_codec_encode(struct bsal_dna_codec *self, int length_in_nucleotides, char *dna_sequence, void *encoded_sequence);
void bsal_dna_codec_decode(struct bsal_dna_codec *self, int length_in_nucleotides, void *encoded_sequence, char*dna_sequence);

char bsal_dna_codec_get_nucleotide(struct bsal_dna_codec *codec, void *encoded_sequence, int index);
int bsal_dna_codec_get_nucleotide_code(struct bsal_dna_codec *codec, void *encoded_sequence, int index);

void bsal_dna_codec_set_nucleotide(void *encoded_sequence, int index, char nucleotide);
uint64_t bsal_dna_codec_get_code(char nucleotide);
char bsal_dna_codec_get_nucleotide_from_code(uint64_t code);

int bsal_dna_codec_encoded_length_default(struct bsal_dna_codec *self, int length_in_nucleotides);
void bsal_dna_codec_decode_default(struct bsal_dna_codec *self, int length_in_nucleotides, void *encoded_sequence, char *dna_sequence);
void bsal_dna_codec_encode_default(struct bsal_dna_codec *self, int length_in_nucleotides, char *dna_sequence, void *encoded_sequence);


void bsal_dna_codec_encode_with_blocks(struct bsal_dna_codec *self,
                int length_in_nucleotides, char *dna_sequence, void *encoded_sequence);

void bsal_dna_codec_generate_blocks(struct bsal_dna_codec *self);

void bsal_dna_codec_generate_block(struct bsal_dna_codec *self, int position, char symbol,
                char *block);
void bsal_dna_codec_decode_with_blocks(struct bsal_dna_codec *self,
                int length_in_nucleotides, void *encoded_sequence, char *dna_sequence);

void bsal_dna_codec_reverse_complement_in_place(struct bsal_dna_codec *codec,
                int length_in_nucleotides, void *encoded_sequence);

void bsal_dna_codec_enable_two_bit_encoding(struct bsal_dna_codec *codec);
void bsal_dna_codec_disable_two_bit_encoding(struct bsal_dna_codec *codec);

int bsal_dna_codec_is_canonical(struct bsal_dna_codec *codec,
                int length_in_nucleotides, void *encoded_sequence);
int bsal_dna_codec_get_complement(int code);


#endif
