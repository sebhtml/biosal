
#ifndef BSAL_DNA_CODEC_H
#define BSAL_DNA_CODEC_H

#include <stdint.h>

#include <core/structures/map.h>

#define BSAL_DNA_CODEC_HAS_REVERSE_COMPLEMENT_IMPLEMENTATION

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

char bsal_dna_codec_get_nucleotide(void *encoded_sequence, int index);
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

#endif
