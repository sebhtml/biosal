
#include "dna_sequence.h"

#include <system/packer.h>

#include <stdlib.h>
#include <string.h>

void bsal_dna_sequence_init(struct bsal_dna_sequence *sequence, void *data)
{
    /* TODO
     * encode @raw_data in 2-bit format
     * use an allocator provided to allocate memory
     */
    sequence->length = strlen((char *)data);
    sequence->data = malloc(sequence->length + 1);

    memcpy(sequence->data, data, sequence->length + 1);
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

    bsal_packer_init(&packer, operation, &buffer);
    bsal_packer_work(&packer, &sequence->pair, sizeof(sequence->pair));

        bsal_packer_work(&packer, &sequence->length, sizeof(sequence->length));

    /* TODO: encode in 2 bits instead !
     */
    if (operation == BSAL_PACKER_OPERATION_UNPACK) {

        if (sequence->length > 0) {
            sequence->data = malloc(sequence->length + 1);
            bsal_packer_work(&packer, sequence->data, sequence->length);
        } else {
            sequence->data = NULL;
        }
    }

    offset = bsal_packer_worked_bytes(&packer);

    bsal_packer_destroy(&packer);

    return offset;
}
