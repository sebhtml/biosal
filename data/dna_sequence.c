
#include "dna_sequence.h"

#include <system/packer.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
#define BSAL_DNA_SEQUENCE_DEBUG
*/
void bsal_dna_sequence_init(struct bsal_dna_sequence *sequence, void *data)
{
    /* TODO
     * encode @raw_data in 2-bit format
     * use an allocator provided to allocate memory
     */
    if (data == NULL) {
        sequence->data = NULL;
        sequence->length = 0;
    } else {
        sequence->length = strlen((char *)data);
        sequence->data = malloc(sequence->length + 1);
        memcpy(sequence->data, data, sequence->length + 1);
    }

    sequence->pair = -1;
}

void bsal_dna_sequence_destroy(struct bsal_dna_sequence *sequence)
{
    if (sequence->data != NULL) {
        free(sequence->data);
        sequence->data = NULL;
    }

    sequence->data = NULL;
    sequence->length = 0;
    sequence->pair = -1;
}

int bsal_dna_sequence_pack_size(struct bsal_dna_sequence *sequence)
{
    return bsal_dna_sequence_pack_unpack(sequence, NULL, BSAL_PACKER_OPERATION_DRY_RUN);
}

int bsal_dna_sequence_unpack(struct bsal_dna_sequence *sequence,
                void *buffer)
{
    return bsal_dna_sequence_pack_unpack(sequence, buffer, BSAL_PACKER_OPERATION_UNPACK);
}

int bsal_dna_sequence_pack(struct bsal_dna_sequence *sequence,
                void *buffer)
{
    return bsal_dna_sequence_pack_unpack(sequence, buffer, BSAL_PACKER_OPERATION_PACK);
}

int bsal_dna_sequence_pack_unpack(struct bsal_dna_sequence *sequence,
                void *buffer, int operation)
{
    struct bsal_packer packer;
    int offset;

#ifdef BSAL_DNA_SEQUENCE_DEBUG
    printf("DEBUG ENTRY bsal_dna_sequence_pack_unpack operation %d\n",
                    operation);

    if (operation == BSAL_PACKER_OPERATION_PACK) {
        bsal_dna_sequence_print(sequence);
    }
#endif

    bsal_packer_init(&packer, operation, buffer);

    /*
    bsal_packer_work(&packer, &sequence->pair, sizeof(sequence->pair));
*/
    bsal_packer_work(&packer, &sequence->length, sizeof(sequence->length));

#ifdef BSAL_DNA_SEQUENCE_DEBUG
    if (operation == BSAL_PACKER_OPERATION_UNPACK) {
        printf("DEBUG bsal_dna_sequence_pack_unpack unpacking: length %d\n",
                        sequence->length);

    } else if (operation == BSAL_PACKER_OPERATION_PACK) {

        printf("DEBUG bsal_dna_sequence_pack_unpack packing: length %d\n",
                        sequence->length);
    }
#endif

    /* TODO: encode in 2 bits instead !
     */
    if (operation == BSAL_PACKER_OPERATION_UNPACK) {

        if (sequence->length > 0) {
            sequence->data = malloc(sequence->length + 1);
        } else {
            sequence->data = NULL;
        }
    }

#ifdef BSAL_DNA_SEQUENCE_DEBUG
    if (operation == BSAL_PACKER_OPERATION_UNPACK) {
        printf("DEBUG unpacking %d bytes\n", sequence->length + 1);
    }
#endif

    if (sequence->length > 0) {
        bsal_packer_work(&packer, sequence->data, sequence->length + 1);
    }

    offset = bsal_packer_worked_bytes(&packer);

    bsal_packer_destroy(&packer);

#ifdef BSAL_DNA_SEQUENCE_DEBUG
    printf("DEBUG EXIT bsal_dna_sequence_pack_unpack operation %d offset %d\n",
                    operation, offset);
#endif

    return offset;
}

void bsal_dna_sequence_print(struct bsal_dna_sequence *self)
{
    char *dna_sequence;

    dna_sequence = self->data;

    printf("DNA: length %d %s\n", self->length, dna_sequence);
}

int bsal_dna_sequence_length(struct bsal_dna_sequence *self)
{
    return self->length;
}

char *bsal_dna_sequence_sequence(struct bsal_dna_sequence *self)
{
    return (char *)self->data;
}

void bsal_dna_sequence_init_copy(struct bsal_dna_sequence *self,
                struct bsal_dna_sequence *other)
{
    bsal_dna_sequence_init(self, other->data);
}
