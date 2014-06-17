
#ifndef BSAL_DNA_KMER_H
#define BSAL_DNA_KMER_H

#include <stdint.h>

struct bsal_dna_kmer {
    void *data;
    int length;
};

void bsal_dna_kmer_init(struct bsal_dna_kmer *sequence,
                void *data);
void bsal_dna_kmer_destroy(struct bsal_dna_kmer *sequence);

int bsal_dna_kmer_unpack(struct bsal_dna_kmer *sequence,
                void *buffer);
int bsal_dna_kmer_pack(struct bsal_dna_kmer *sequence,
                void *buffer);
int bsal_dna_kmer_pack_size(struct bsal_dna_kmer *sequence);
int bsal_dna_kmer_pack_unpack(struct bsal_dna_kmer *sequence,
                void *buffer, int operation);

int bsal_dna_kmer_length(struct bsal_dna_kmer *self);
void bsal_dna_kmer_init_mock(struct bsal_dna_kmer *sequence, int kmer_length);
void bsal_dna_kmer_init_copy(struct bsal_dna_kmer *sequence, struct bsal_dna_kmer *other);

void bsal_dna_kmer_print(struct bsal_dna_kmer *sequence);
char *bsal_dna_kmer_sequence(struct bsal_dna_kmer *sequence);

int bsal_dna_kmer_store_index(struct bsal_dna_kmer *self, int stores);
int bsal_dna_kmer_pack_store_key(struct bsal_dna_kmer *sequence,
                void *buffer);

char bsal_dna_kmer_complement(char nucleotide);
void bsal_dna_kmer_reverse_complement(char *sequence1, char *sequence2);
void bsal_dna_kmer_normalize(char *sequence);
char bsal_dna_kmer_normalize_nucleotide(char nucleotide);

#endif
