
#ifndef BIOSAL_DNA_CODEC_H
#define BIOSAL_DNA_CODEC_H

#include <stdint.h>

#include <core/system/memory_pool.h>
#include <core/structures/map.h>

#define BIOSAL_DNA_CODEC_HAS_REVERSE_COMPLEMENT_IMPLEMENTATION

/*
*/

#define BIOSAL_NUCLEOTIDE_CODE_A 0 /* ~00 == 11 */
#define BIOSAL_NUCLEOTIDE_CODE_C 1 /* ~01 == 10 */
#define BIOSAL_NUCLEOTIDE_CODE_G 2 /* ~10 == 01 */
#define BIOSAL_NUCLEOTIDE_CODE_T 3 /* ~11 == 00 */

#define BIOSAL_NUCLEOTIDE_SYMBOL_A 'A'
#define BIOSAL_NUCLEOTIDE_SYMBOL_C 'C'
#define BIOSAL_NUCLEOTIDE_SYMBOL_G 'G'
#define BIOSAL_NUCLEOTIDE_SYMBOL_T 'T'

/*
 * use block encoder (faster or not ?)
#define BIOSAL_DNA_CODEC_USE_TWO_BIT_BLOCK_ENCODER
 */

/*
 * use block decoder (faster or not ?)
#define BIOSAL_DNA_CODEC_USE_TWO_BIT_BLOCK_DECODER
 */

/*
 * A class to encode and decode DNA data.
 */
struct biosal_dna_codec {
#ifdef BIOSAL_DNA_CODEC_USE_TWO_BIT_BLOCK_ENCODER
    struct biosal_map encoding_lookup_table;
#endif
#ifdef BIOSAL_DNA_CODEC_USE_TWO_BIT_BLOCK_DECODER
    struct biosal_map decoding_lookup_table;
#endif
    int block_length;

    int use_two_bit_encoding;
    struct biosal_memory_pool pool;
};

void biosal_dna_codec_init(struct biosal_dna_codec *self);
void biosal_dna_codec_destroy(struct biosal_dna_codec *self);

int biosal_dna_codec_encoded_length(struct biosal_dna_codec *self, int length_in_nucleotides);
void biosal_dna_codec_encode(struct biosal_dna_codec *self, int length_in_nucleotides, char *dna_sequence, void *encoded_sequence);
void biosal_dna_codec_decode(struct biosal_dna_codec *self, int length_in_nucleotides, void *encoded_sequence, char*dna_sequence);

char biosal_dna_codec_get_nucleotide(struct biosal_dna_codec *codec, void *encoded_sequence, int index);
int biosal_dna_codec_get_nucleotide_code(struct biosal_dna_codec *codec, void *encoded_sequence, int index);

void biosal_dna_codec_set_nucleotide(struct biosal_dna_codec *self,
                void *encoded_sequence, int index, char nucleotide);
uint64_t biosal_dna_codec_get_code(char nucleotide);
char biosal_dna_codec_get_nucleotide_from_code(uint64_t code);

int biosal_dna_codec_encoded_length_default(struct biosal_dna_codec *self, int length_in_nucleotides);
void biosal_dna_codec_decode_default(struct biosal_dna_codec *self, int length_in_nucleotides, void *encoded_sequence, char *dna_sequence);
void biosal_dna_codec_encode_default(struct biosal_dna_codec *self, int length_in_nucleotides, char *dna_sequence, void *encoded_sequence);

#ifdef BIOSAL_DNA_CODEC_USE_TWO_BIT_BLOCK_ENCODER
void biosal_dna_codec_encode_with_blocks(struct biosal_dna_codec *self,
                int length_in_nucleotides, char *dna_sequence, void *encoded_sequence);

void biosal_dna_codec_generate_blocks(struct biosal_dna_codec *self);

void biosal_dna_codec_generate_block(struct biosal_dna_codec *self, int position, char symbol,
                char *block);
#endif

#ifdef BIOSAL_DNA_CODEC_USE_TWO_BIT_BLOCK_DECODER
void biosal_dna_codec_decode_with_blocks(struct biosal_dna_codec *self,
                int length_in_nucleotides, void *encoded_sequence, char *dna_sequence);
#endif

void biosal_dna_codec_reverse_complement_in_place(struct biosal_dna_codec *codec,
                int length_in_nucleotides, void *encoded_sequence);

void biosal_dna_codec_enable_two_bit_encoding(struct biosal_dna_codec *codec);
void biosal_dna_codec_disable_two_bit_encoding(struct biosal_dna_codec *codec);

int biosal_dna_codec_is_canonical(struct biosal_dna_codec *codec,
                int length_in_nucleotides, void *encoded_sequence);
int biosal_dna_codec_get_complement(int code);

void biosal_dna_codec_mutate_as_child(struct biosal_dna_codec *self,
                int length_in_nucleotides, void *encoded_sequence, int last_code);
void biosal_dna_codec_mutate_as_parent(struct biosal_dna_codec *self,
                int length_in_nucleotides, void *encoded_sequence, int first_code);
int biosal_dna_codec_must_use_two_bit_encoding(struct biosal_dna_codec *self,
                int node_count);

#endif
