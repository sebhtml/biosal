
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
        sequence->length = strlen((char *)data);
        sequence->data = malloc(sequence->length + 1);
        memcpy(sequence->data, data, sequence->length + 1);
    }
}

void bsal_dna_kmer_destroy(struct bsal_dna_kmer *sequence)
{
    if (sequence->data != NULL) {
        free(sequence->data);
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
            sequence->data = malloc(sequence->length + 1);
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

    sequence->data = malloc(sequence->length + 1);
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

    seed = 0xcaa9cfcf;

    value = bsal_murmur_hash_2_64_a(self->data, self->length, seed);

    return value % stores;
}
