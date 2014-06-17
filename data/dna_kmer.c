
#include "dna_kmer.h"

#include <system/packer.h>

#include <system/memory.h>

#include <hash/murmur_hash_2_64_a.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <stdint.h>

/*
#define BSAL_DNA_SEQUENCE_DEBUG
*/
void bsal_dna_kmer_init(struct bsal_dna_kmer *sequence, void *data)
{
    if (data == NULL) {
        sequence->data = NULL;
        sequence->length = 0;
    } else {
        bsal_dna_kmer_normalize(data);
        sequence->length = strlen((char *)data);
        sequence->data = bsal_malloc(sequence->length + 1);
        memcpy(sequence->data, data, sequence->length + 1);
    }
}

void bsal_dna_kmer_destroy(struct bsal_dna_kmer *sequence)
{
    if (sequence->data != NULL) {
        bsal_free(sequence->data);
        sequence->data = NULL;
    }

    sequence->data = NULL;
    sequence->length = 0;
}

int bsal_dna_kmer_pack_size(struct bsal_dna_kmer *sequence)
{
    return bsal_dna_kmer_pack_unpack(sequence, NULL, BSAL_PACKER_OPERATION_DRY_RUN);
}

int bsal_dna_kmer_unpack(struct bsal_dna_kmer *sequence,
                void *buffer)
{
    return bsal_dna_kmer_pack_unpack(sequence, buffer, BSAL_PACKER_OPERATION_UNPACK);
}

int bsal_dna_kmer_pack_store_key(struct bsal_dna_kmer *self,
                void *buffer)
{
    struct bsal_dna_kmer kmer2;
    char *reverse_complement_sequence;
    char *self_sequence;
    int bytes;

    self_sequence = self->data;
    reverse_complement_sequence = bsal_malloc(self->length + 1);

    bsal_dna_kmer_reverse_complement(self_sequence, reverse_complement_sequence);

    if (strcmp(reverse_complement_sequence, self_sequence) < 0) {

        bsal_dna_kmer_init(&kmer2, reverse_complement_sequence);
        bytes = bsal_dna_kmer_pack(&kmer2, buffer);
        bsal_dna_kmer_destroy(&kmer2);

    } else {
        bytes = bsal_dna_kmer_pack(self, buffer);
    }

    bsal_free(reverse_complement_sequence);
    reverse_complement_sequence = NULL;

    return bytes;
}

int bsal_dna_kmer_pack(struct bsal_dna_kmer *sequence,
                void *buffer)
{
    return bsal_dna_kmer_pack_unpack(sequence, buffer, BSAL_PACKER_OPERATION_PACK);
}

int bsal_dna_kmer_pack_unpack(struct bsal_dna_kmer *sequence,
                void *buffer, int operation)
{
    struct bsal_packer packer;
    int offset;

    bsal_packer_init(&packer, operation, buffer);

    bsal_packer_work(&packer, &sequence->length, sizeof(sequence->length));

    /* TODO: encode in 2 bits instead !
     */
    if (operation == BSAL_PACKER_OPERATION_UNPACK) {

        if (sequence->length > 0) {
            sequence->data = bsal_malloc(sequence->length + 1);
        } else {
            sequence->data = NULL;
        }
    }

    if (sequence->length > 0) {
        bsal_packer_work(&packer, sequence->data, sequence->length + 1);
    }

    offset = bsal_packer_worked_bytes(&packer);

    bsal_packer_destroy(&packer);

    return offset;
}

int bsal_dna_kmer_length(struct bsal_dna_kmer *self)
{
    return self->length;
}

void bsal_dna_kmer_init_mock(struct bsal_dna_kmer *sequence, int kmer_length)
{
    char *dna;
    int i;

    dna = (char *)bsal_malloc(kmer_length + 1);

    for (i = 0; i < kmer_length; i++) {
        dna[i] = 'A';
    }

    dna[kmer_length] = '\0';

    bsal_dna_kmer_init(sequence, dna);
    bsal_free(dna);
}

void bsal_dna_kmer_init_copy(struct bsal_dna_kmer *sequence, struct bsal_dna_kmer *other)
{
    sequence->length = other->length;

    sequence->data = bsal_malloc(sequence->length + 1);
    memcpy(sequence->data, other->data, sequence->length + 1);
}

void bsal_dna_kmer_print(struct bsal_dna_kmer *self)
{
    printf("KMER length: %d nucleotides, sequence: %s\n", self->length,
                    (char *)self->data);
}

int bsal_dna_kmer_store_index(struct bsal_dna_kmer *self, int stores)
{
    uint64_t value;
    int seed;
    char *reverse_complement_sequence;
    int store_index;
    char *self_sequence;
    struct bsal_dna_kmer kmer2;

    seed = 0xcaa9cfcf;

    self_sequence = self->data;
    reverse_complement_sequence = bsal_malloc(self->length + 1);

    bsal_dna_kmer_reverse_complement(self_sequence, reverse_complement_sequence);

    if (strcmp(reverse_complement_sequence, self_sequence) < 0) {

        bsal_dna_kmer_init(&kmer2, reverse_complement_sequence);
        value = bsal_murmur_hash_2_64_a(kmer2.data, kmer2.length, seed);
        bsal_dna_kmer_destroy(&kmer2);

    } else {

        value = bsal_murmur_hash_2_64_a(self->data, self->length, seed);
    }

    store_index = value % stores;

    bsal_free(reverse_complement_sequence);
    reverse_complement_sequence = NULL;

    return store_index;
}

char *bsal_dna_kmer_sequence(struct bsal_dna_kmer *sequence)
{
    return sequence->data;
}

void bsal_dna_kmer_reverse_complement(char *sequence1, char *sequence2)
{
    int length;
    int position1;
    int position2;
    char nucleotide1;
    char nucleotide2;

    length = strlen(sequence1);

    position1 = length - 1;
    position2 = 0;

    while (position2 < length) {

        nucleotide1 = sequence1[position1];
        nucleotide2 = bsal_dna_kmer_complement(nucleotide1);

        sequence2[position2] = nucleotide2;

        position2++;
        position1--;
    }

    sequence2[position2] = '\0';

#ifdef BSAL_DNA_KMER_DEBUG
    printf("%s and %s\n", sequence1, sequence2);
#endif
}

char bsal_dna_kmer_complement(char nucleotide)
{
    switch (nucleotide) {
        case 'A':
            return 'T';
        case 'T':
            return 'A';
        case 'C':
            return 'G';
        case 'G':
            return 'C';
        default:
            return nucleotide;
    }

    return nucleotide;
}

void bsal_dna_kmer_normalize(char *sequence)
{
    int length;
    int position;

    length = strlen(sequence);

    position = 0;

    while (position < length) {

        sequence[position] = bsal_dna_kmer_normalize_nucleotide(sequence[position]);

        position++;
    }
}

char bsal_dna_kmer_normalize_nucleotide(char nucleotide)
{
    switch (nucleotide) {
        case 't':
            return 'T';
        case 'a':
            return 'A';
        case 'g':
            return 'G';
        case 'c':
            return 'C';
        case 'T':
            return 'T';
        case 'A':
            return 'A';
        case 'G':
            return 'G';
        case 'C':
            return 'C';
        case 'n':
            return 'N';
        case '.':
            return 'N';
        default:
            return 'N';
    }

    return 'N';
}


