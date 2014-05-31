
#ifndef _BSAL_DNA_SEQUENCE_H
#define _BSAL_DNA_SEQUENCE_H

struct bsal_dna_sequence {
    int foo;
};

void bsal_dna_sequence_init(struct bsal_dna_sequence *proxy);
void bsal_dna_sequence_destroy(struct bsal_dna_sequence *proxy);

#endif
