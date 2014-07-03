
#ifndef BSAL_DNA_SEQUENCE_H
#define BSAL_DNA_SEQUENCE_H

#include <stdint.h>

struct bsal_dna_codec;
struct bsal_memory_pool;

struct bsal_dna_sequence {
    void *encoded_data;
    int length_in_nucleotides;
    int64_t pair;
};

void bsal_dna_sequence_init(struct bsal_dna_sequence *sequence,
                char *data, struct bsal_dna_codec *codec, struct bsal_memory_pool *memory);
void bsal_dna_sequence_destroy(struct bsal_dna_sequence *sequence, struct bsal_memory_pool *memory);

int bsal_dna_sequence_unpack(struct bsal_dna_sequence *sequence,
                void *buffer, struct bsal_memory_pool *memory, struct bsal_dna_codec *codec);
int bsal_dna_sequence_pack(struct bsal_dna_sequence *sequence,
                void *buffer, struct bsal_dna_codec *codec);
int bsal_dna_sequence_pack_size(struct bsal_dna_sequence *sequence, struct bsal_dna_codec *codec);
int bsal_dna_sequence_pack_unpack(struct bsal_dna_sequence *sequence,
                void *buffer, int operation, struct bsal_memory_pool *memory,
                struct bsal_dna_codec *codec);

void bsal_dna_sequence_print(struct bsal_dna_sequence *sequence, struct bsal_dna_codec *codec,
                struct bsal_memory_pool *memory);

int bsal_dna_sequence_length(struct bsal_dna_sequence *self);

void bsal_dna_sequence_get_sequence(struct bsal_dna_sequence *self, char *sequence,
                struct bsal_dna_codec *codec);
void bsal_dna_sequence_init_same_data(struct bsal_dna_sequence *self,
                struct bsal_dna_sequence *other);
void bsal_dna_sequence_init_copy(struct bsal_dna_sequence *self,
                struct bsal_dna_sequence *other, struct bsal_dna_codec *codec,
                struct bsal_memory_pool *memory);

#endif
