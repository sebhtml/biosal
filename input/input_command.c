
#include "input_command.h"

#include <system/packer.h>
#include <data/dna_sequence.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <inttypes.h>

/*
#define BSAL_INPUT_COMMAND_DEBUG
*/
void bsal_input_command_init(struct bsal_input_command *self,
                int store_name, uint64_t store_first, uint64_t store_last)
{
    self->store_name = store_name;
    self->store_first = store_first;
    self->store_last = store_last;

    bsal_vector_init(&self->entries, sizeof(struct bsal_dna_sequence));
}

void bsal_input_command_destroy(struct bsal_input_command *self)
{
    self->store_name= -1;
    self->store_first = 0;
    self->store_last = 0;

    bsal_vector_destroy(&self->entries);
}

int bsal_input_command_store_name(struct bsal_input_command *self)
{
    return self->store_name;
}

int bsal_input_command_pack_size(struct bsal_input_command *self)
{
    return bsal_input_command_pack_unpack(self, NULL, BSAL_PACKER_OPERATION_DRY_RUN);
}

void bsal_input_command_pack(struct bsal_input_command *self, void *buffer)
{
    bsal_input_command_pack_unpack(self, buffer, BSAL_PACKER_OPERATION_PACK);
}

void bsal_input_command_unpack(struct bsal_input_command *self, void *buffer)
{
#ifdef BSAL_INPUT_COMMAND_DEBUG
    printf("DEBUG bsal_input_command_unpack %p\n", buffer);
#endif

    bsal_input_command_pack_unpack(self, buffer, BSAL_PACKER_OPERATION_UNPACK);
}

uint64_t bsal_input_command_store_first(struct bsal_input_command *self)
{
    return self->store_first;
}

uint64_t bsal_input_command_store_last(struct bsal_input_command *self)
{
    return self->store_last;
}

int bsal_input_command_pack_unpack(struct bsal_input_command *self, void *buffer,
                int operation)
{
    struct bsal_packer packer;
    int offset;
    int entries;
    struct bsal_dna_sequence dna_sequence;

#ifdef BSAL_INPUT_COMMAND_DEBUG
    if (operation == BSAL_PACKER_OPERATION_UNPACK) {
        printf("DEBUG bsal_input_command_pack_unpack operation %d\n",
                        operation);
    }
#endif

    offset = 0;

    bsal_packer_init(&packer, operation, buffer);

    bsal_packer_work(&packer, &self->store_name, sizeof(self->store_name));
    bsal_packer_work(&packer, &self->store_first, sizeof(self->store_first));
    bsal_packer_work(&packer, &self->store_last, sizeof(self->store_last));

    /* TODO remove this line
     */

    if (operation == BSAL_PACKER_OPERATION_UNPACK) {
        bsal_vector_init(&self->entries, sizeof(struct bsal_dna_sequence));
    }

    return bsal_packer_worked_bytes(&packer);

#ifdef BSAL_INPUT_COMMAND_DEBUG
    if (operation == BSAL_PACKER_OPERATION_UNPACK) {
        printf("DEBUG bsal_input_command_pack_unpack offset after packing stuff %d\n",
                    offset);
    }
#endif

    /* do the vector too
     * process the count.
     * then, process every read.
     */

    /* 1. entries
     */
    bsal_packer_work(&packer, &entries, sizeof(entries));

    offset += bsal_packer_worked_bytes(&packer);
    bsal_packer_destroy(&packer);

    /* 2. get the entries
     */
    if (operation == BSAL_PACKER_OPERATION_UNPACK) {
        bsal_vector_init(&self->entries, sizeof(struct bsal_dna_sequence));

        while (entries--) {
            offset += bsal_dna_sequence_unpack(&dna_sequence,
                            (char *) buffer + offset);
        }
    } else if (operation == BSAL_PACKER_OPERATION_PACK) {

        while (entries--) {
            offset += bsal_dna_sequence_pack(&dna_sequence,
                            (char *) buffer + offset);
        }
    } else if (operation == BSAL_PACKER_OPERATION_DRY_RUN) {

        while (entries--) {
            offset += bsal_dna_sequence_pack_size(&dna_sequence);
        }
    }

#ifdef BSAL_INPUT_COMMAND_DEBUG
    if (operation == BSAL_PACKER_OPERATION_UNPACK) {
        printf("DEBUG bsal_input_command_pack_unpack final offset %d\n",
                    offset);
    }
#endif

    return offset;
}

void bsal_input_command_print(struct bsal_input_command *self)
{
    printf("[===] input command: store_name %d store_first %" PRIu64 " store_last %" PRIu64 "\n",
            self->store_name, self->store_first, self->store_last);
}

struct bsal_vector *bsal_input_command_entries(struct bsal_input_command *self)
{
    return &self->entries;
}
