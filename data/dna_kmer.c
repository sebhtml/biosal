
#include "dna_kmer.h"

#include "dna_codec.h"

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
void bsal_dna_kmer_init(struct bsal_dna_kmer *sequence, char *data)
{
    int encoded_length;

    if (data == NULL) {
        sequence->encoded_data = NULL;
        sequence->length_in_bases = 0;
    } else {
        bsal_dna_kmer_normalize(data);

        sequence->length_in_bases = strlen((char *)data);

        encoded_length = bsal_dna_codec_encoded_length(sequence->length_in_bases);
        sequence->encoded_data = bsal_malloc(encoded_length);
        bsal_dna_codec_encode(sequence->length_in_bases, data, sequence->encoded_data);
    }
}

void bsal_dna_kmer_destroy(struct bsal_dna_kmer *sequence)
{
    if (sequence->encoded_data != NULL) {
        bsal_free(sequence->encoded_data);
        sequence->encoded_data = NULL;
    }

    sequence->encoded_data = NULL;
    sequence->length_in_bases = 0;
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

    self_sequence = bsal_malloc(self->length_in_bases + 1);

    bsal_dna_codec_decode(self->length_in_bases, self->encoded_data, self_sequence);

    reverse_complement_sequence = bsal_malloc(self->length_in_bases + 1);

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
    bsal_free(self_sequence);
    self_sequence = NULL;

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
    int encoded_length;

    bsal_packer_init(&packer, operation, buffer);

    bsal_packer_work(&packer, &sequence->length_in_bases, sizeof(sequence->length_in_bases));

    encoded_length = bsal_dna_codec_encoded_length(sequence->length_in_bases);

    /* TODO: encode in 2 bits instead !
     */
    if (operation == BSAL_PACKER_OPERATION_UNPACK) {

        if (sequence->length_in_bases > 0) {
            sequence->encoded_data = bsal_malloc(encoded_length);
        } else {
            sequence->encoded_data = NULL;
        }
    }

    if (sequence->length_in_bases > 0) {
        bsal_packer_work(&packer, sequence->encoded_data, encoded_length);
    }

    offset = bsal_packer_worked_bytes(&packer);

    bsal_packer_destroy(&packer);

    return offset;
}

int bsal_dna_kmer_length(struct bsal_dna_kmer *self)
{
    return self->length_in_bases;
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
    char *dna_sequence;

    dna_sequence = bsal_malloc(other->length_in_bases + 1);

    bsal_dna_codec_decode(other->length_in_bases, other->encoded_data,
                    dna_sequence);

    bsal_dna_kmer_init(sequence, dna_sequence);

    bsal_free(dna_sequence);
    dna_sequence = NULL;
}

void bsal_dna_kmer_print(struct bsal_dna_kmer *self)
{
    char *dna_sequence;

    dna_sequence = bsal_malloc(self->length_in_bases + 1);

    bsal_dna_codec_decode(self->length_in_bases, self->encoded_data, dna_sequence);

    printf("KMER length: %d nucleotides, sequence: %s\n", self->length_in_bases,
                   dna_sequence);

    bsal_free(dna_sequence);
    dna_sequence = NULL;
}

uint64_t bsal_dna_kmer_hash(struct bsal_dna_kmer *self)
{
    unsigned int seed;
    seed = 0xcaa9cfcf;

    return bsal_murmur_hash_2_64_a(self->encoded_data, self->length_in_bases, seed);
}
int bsal_dna_kmer_store_index(struct bsal_dna_kmer *self, int stores)
{
    uint64_t value;
    char *reverse_complement_sequence;
    int store_index;
    char *self_sequence;
    struct bsal_dna_kmer kmer2;


    self_sequence = bsal_malloc(self->length_in_bases + 1);
    reverse_complement_sequence = bsal_malloc(self->length_in_bases + 1);

    bsal_dna_kmer_reverse_complement(self_sequence, reverse_complement_sequence);

    if (strcmp(reverse_complement_sequence, self_sequence) < 0) {

        bsal_dna_kmer_init(&kmer2, reverse_complement_sequence);
        value = bsal_dna_kmer_hash(&kmer2);
        bsal_dna_kmer_destroy(&kmer2);

    } else {

        value = bsal_dna_kmer_hash(self);
    }

    store_index = value % stores;

    bsal_free(reverse_complement_sequence);
    reverse_complement_sequence = NULL;

    bsal_free(self_sequence);
    self_sequence = NULL;

    return store_index;
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

void bsal_dna_kmer_get_sequence(struct bsal_dna_kmer *self, char *sequence)
{
    bsal_dna_codec_decode(self->length_in_bases, self->encoded_data, sequence);
}
