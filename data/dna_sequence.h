
#ifndef _BSAL_DNA_SEQUENCE_H
#define _BSAL_DNA_SEQUENCE_H

#include <stdint.h>

struct bsal_dna_sequence {
    void *data;
    int length;
    int64_t pair;
};

void bsal_dna_sequence_init(struct bsal_dna_sequence *sequence,
                char *raw_data);
void bsal_dna_sequence_destroy(struct bsal_dna_sequence *sequence);

#endif
