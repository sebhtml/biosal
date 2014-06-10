
#ifndef BSAL_DNA_SEQUENCE_H
#define BSAL_DNA_SEQUENCE_H

#include <stdint.h>

struct bsal_dna_sequence {
    void *data;
    int length;
    int64_t pair;
};

void bsal_dna_sequence_init(struct bsal_dna_sequence *sequence,
                void *raw_data);
void bsal_dna_sequence_destroy(struct bsal_dna_sequence *sequence);

int bsal_dna_sequence_unpack(struct bsal_dna_sequence *sequence,
                void *buffer);
int bsal_dna_sequence_pack(struct bsal_dna_sequence *sequence,
                void *buffer);
int bsal_dna_sequence_pack_size(struct bsal_dna_sequence *sequence);
int bsal_dna_sequence_pack_unpack(struct bsal_dna_sequence *sequence,
                void *buffer, int operation);

#endif
