
#ifndef BSAL_DNA_SEQUENCE_H
#define BSAL_DNA_SEQUENCE_H

#include <stdint.h>

struct bsal_dna_sequence {
    int length;
    int64_t pair;
    void *data;
};

void bsal_dna_sequence_init(struct bsal_dna_sequence *sequence,
                void *data);
void bsal_dna_sequence_destroy(struct bsal_dna_sequence *sequence);

int bsal_dna_sequence_unpack(struct bsal_dna_sequence *sequence,
                void *buffer);
int bsal_dna_sequence_pack(struct bsal_dna_sequence *sequence,
                void *buffer);
int bsal_dna_sequence_pack_size(struct bsal_dna_sequence *sequence);
int bsal_dna_sequence_pack_unpack(struct bsal_dna_sequence *sequence,
                void *buffer, int operation);

void bsal_dna_sequence_print(struct bsal_dna_sequence *sequence);

int bsal_dna_sequence_length(struct bsal_dna_sequence *self);

char *bsal_dna_sequence_sequence(struct bsal_dna_sequence *self);
void bsal_dna_sequence_init_same_data(struct bsal_dna_sequence *self,
                struct bsal_dna_sequence *other);
void bsal_dna_sequence_init_copy(struct bsal_dna_sequence *self,
                struct bsal_dna_sequence *other);

#endif
