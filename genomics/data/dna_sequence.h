
#ifndef BIOSAL_DNA_SEQUENCE_H
#define BIOSAL_DNA_SEQUENCE_H

#include <stdint.h>

struct biosal_dna_codec;
struct biosal_memory_pool;

struct biosal_dna_sequence {
    void *encoded_data;
    int length_in_nucleotides;
    int64_t pair;
};

void biosal_dna_sequence_init(struct biosal_dna_sequence *sequence,
                char *data, struct biosal_dna_codec *codec, struct biosal_memory_pool *memory);
void biosal_dna_sequence_destroy(struct biosal_dna_sequence *sequence, struct biosal_memory_pool *memory);

int biosal_dna_sequence_unpack(struct biosal_dna_sequence *sequence,
                void *buffer, struct biosal_memory_pool *memory, struct biosal_dna_codec *codec);
int biosal_dna_sequence_pack(struct biosal_dna_sequence *sequence,
                void *buffer, struct biosal_dna_codec *codec);
int biosal_dna_sequence_pack_size(struct biosal_dna_sequence *sequence, struct biosal_dna_codec *codec);
int biosal_dna_sequence_pack_unpack(struct biosal_dna_sequence *sequence,
                void *buffer, int operation, struct biosal_memory_pool *memory,
                struct biosal_dna_codec *codec);

void biosal_dna_sequence_print(struct biosal_dna_sequence *sequence, struct biosal_dna_codec *codec,
                struct biosal_memory_pool *memory);

int biosal_dna_sequence_length(struct biosal_dna_sequence *self);

void biosal_dna_sequence_get_sequence(struct biosal_dna_sequence *self, char *sequence,
                struct biosal_dna_codec *codec);
void biosal_dna_sequence_init_same_data(struct biosal_dna_sequence *self,
                struct biosal_dna_sequence *other);
void biosal_dna_sequence_init_copy(struct biosal_dna_sequence *self,
                struct biosal_dna_sequence *other, struct biosal_dna_codec *codec,
                struct biosal_memory_pool *memory);

#endif
