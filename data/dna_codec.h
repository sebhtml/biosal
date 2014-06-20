
#ifndef BSAL_DNA_CODEC_H
#define BSAL_DNA_CODEC_H

#include <stdint.h>

#include <structures/map.h>

struct bsal_dna_codec {
    struct bsal_map encoding_lookup_table;
    struct bsal_map decoding_lookup_table;
    int block_length;
};

void bsal_dna_codec_init(struct bsal_dna_codec *self);
void bsal_dna_codec_destroy(struct bsal_dna_codec *self);

int bsal_dna_codec_encoded_length(int length_in_nucleotides);
void bsal_dna_codec_encode(struct bsal_dna_codec *self, int length_in_nucleotides, char *dna_sequence, void *encoded_sequence);
void bsal_dna_codec_decode(struct bsal_dna_codec *self, int length_in_nucleotides, void *encoded_sequence, char*dna_sequence);

char bsal_dna_codec_get_nucleotide(void *encoded_sequence, int index);
void bsal_dna_codec_set_nucleotide(void *encoded_sequence, int index, char nucleotide);
uint64_t bsal_dna_codec_get_code(char nucleotide);
char bsal_dna_codec_get_nucleotide_from_code(uint64_t code);

int bsal_dna_codec_encoded_length_default(int length_in_nucleotides);
void bsal_dna_codec_decode_default(int length_in_nucleotides, void *encoded_sequence, char *dna_sequence);
void bsal_dna_codec_encode_default(int length_in_nucleotides, char *dna_sequence, void *encoded_sequence);


void bsal_dna_codec_encode_with_blocks(struct bsal_dna_codec *self,
                int length_in_nucleotides, char *dna_sequence, void *encoded_sequence);

void bsal_dna_codec_generate_blocks(struct bsal_dna_codec *self);

void bsal_dna_codec_generate_block(struct bsal_dna_codec *self, int position, char symbol,
                char *block);
#endif
