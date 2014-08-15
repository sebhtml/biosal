
#include "input_command.h"

#include <core/system/packer.h>
#include <core/structures/vector_iterator.h>
#include <genomics/data/dna_sequence.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

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

void bsal_input_command_destroy(struct bsal_input_command *self, struct bsal_memory_pool *memory)
{
    struct bsal_dna_sequence *sequence;
    struct bsal_vector_iterator iterator;
    self->store_name= -1;
    self->store_first = 0;
    self->store_last = 0;

    bsal_vector_iterator_init(&iterator, &self->entries);

    while (bsal_vector_iterator_has_next(&iterator)) {
        bsal_vector_iterator_next(&iterator, (void **)&sequence);

        bsal_dna_sequence_destroy(sequence, memory);
    }

    bsal_vector_iterator_destroy(&iterator);

    bsal_vector_destroy(&self->entries);
}

int bsal_input_command_store_name(struct bsal_input_command *self)
{
    return self->store_name;
}

int bsal_input_command_pack_size(struct bsal_input_command *self,
                struct bsal_dna_codec *codec)
{
    return bsal_input_command_pack_unpack(self, NULL, BSAL_PACKER_OPERATION_DRY_RUN, NULL, codec);
}

int bsal_input_command_pack(struct bsal_input_command *self, void *buffer,
                struct bsal_dna_codec *codec)
{
    return bsal_input_command_pack_unpack(self, buffer, BSAL_PACKER_OPERATION_PACK, NULL, codec);
}

int bsal_input_command_unpack(struct bsal_input_command *self, void *buffer, struct bsal_memory_pool *memory,
                struct bsal_dna_codec *codec)
{
#ifdef BSAL_INPUT_COMMAND_DEBUG
    printf("DEBUG bsal_input_command_unpack %p\n", buffer);
#endif

    return bsal_input_command_pack_unpack(self, buffer, BSAL_PACKER_OPERATION_UNPACK, memory, codec);
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
                int operation, struct bsal_memory_pool *memory, struct bsal_dna_codec *codec)
{
    struct bsal_packer packer;
    int offset;
    int64_t entries;
    struct bsal_dna_sequence dna_sequence;
    struct bsal_dna_sequence *other_sequence;
    int bytes;
    int i;

#ifdef BSAL_INPUT_COMMAND_DEBUG
    if (1 || operation == BSAL_PACKER_OPERATION_UNPACK) {
        printf("DEBUG ENTRY bsal_input_command_pack_unpack operation %d\n",
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

    /*
    if (operation == BSAL_PACKER_OPERATION_UNPACK) {
        bsal_vector_init(&self->entries, sizeof(struct bsal_dna_sequence));
    }

    return bsal_packer_worked_bytes(&packer);
*/

#ifdef BSAL_INPUT_COMMAND_DEBUG
    if (1 || operation == BSAL_PACKER_OPERATION_UNPACK) {
        printf("DEBUG bsal_input_command_pack_unpack offset after packing stuff %d\n",
                    offset);
    }
#endif

    /* do the vector too
     * process the count.
     * then, process every read.
     */

    if (operation == BSAL_PACKER_OPERATION_PACK
                    || operation == BSAL_PACKER_OPERATION_DRY_RUN) {
        entries = bsal_vector_size(&self->entries);
    } else if (operation == BSAL_PACKER_OPERATION_UNPACK) {

        bsal_vector_init(&self->entries, sizeof(struct bsal_dna_sequence));

        bsal_vector_set_memory_pool(&self->entries, memory);
    }

    /* 1. entries
     */
    bsal_packer_work(&packer, &entries, sizeof(entries));

    offset = bsal_packer_worked_bytes(&packer);
    bsal_packer_destroy(&packer);

#ifdef BSAL_INPUT_COMMAND_DEBUG
    printf("DEBUG bsal_input_command_pack_unpack operation %d entries %d offset %d\n",
                    operation, entries, offset);
#endif

    /* 2. get the entries
     */
    if (operation == BSAL_PACKER_OPERATION_UNPACK) {

        while (entries--) {
            bytes = bsal_dna_sequence_unpack(&dna_sequence,
                            (char *) buffer + offset,
                            memory, codec);

#ifdef BSAL_INPUT_COMMAND_DEBUG
            printf("DEBUG unpacking DNA sequence, used %d bytes\n",
                            bytes);
#endif
            offset += bytes;

            bsal_vector_push_back(&self->entries, &dna_sequence);
        }
    } else if (operation == BSAL_PACKER_OPERATION_PACK) {

        i = 0;
        while (entries--) {
            other_sequence = (struct bsal_dna_sequence *)bsal_vector_at(&self->entries,
                            i++);
            bytes = bsal_dna_sequence_pack(other_sequence,
                            (char *) buffer + offset,
                            codec);
            offset += bytes;

#ifdef BSAL_INPUT_COMMAND_DEBUG
            printf("DEBUG packing DNA sequence, used %d bytes, sum %d\n",
                            bytes, offset);
#endif
        }
    } else if (operation == BSAL_PACKER_OPERATION_DRY_RUN) {

        i = 0;
        while (entries--) {
            other_sequence = (struct bsal_dna_sequence *)bsal_vector_at(&self->entries,
                            i++);
            bytes = bsal_dna_sequence_pack_size(other_sequence, codec);

            offset += bytes;

#ifdef BSAL_INPUT_COMMAND_DEBUG
            printf("DEBUG dry run DNA sequence, used %d bytes, sum %d\n",
                            bytes, offset);
#endif
        }
    }

#ifdef BSAL_INPUT_COMMAND_DEBUG
    printf("DEBUG bsal_input_command_pack_unpack operation %d final offset %d\n",
                    operation, offset);

    if (operation == BSAL_PACKER_OPERATION_UNPACK) {
        printf("DEBUG bsal_input_command_pack_unpack unpacked %d entries\n",
                        (int)bsal_vector_size(&self->entries));
    }
#endif

    return offset;
}

void bsal_input_command_print(struct bsal_input_command *self,
                struct bsal_dna_codec *codec)
{
    printf("[===] input command: store_name %d store_first %" PRIu64 " store_last %" PRIu64 ""
                    " entries %d bytes %d\n",
            self->store_name, self->store_first, self->store_last,
            (int)bsal_vector_size(&self->entries), bsal_input_command_pack_size(self, codec));
}

struct bsal_vector *bsal_input_command_entries(struct bsal_input_command *self)
{
    return &self->entries;
}

int bsal_input_command_entry_count(struct bsal_input_command *self)
{
    return bsal_vector_size(&self->entries);
}

void bsal_input_command_add_entry(struct bsal_input_command *self,
                struct bsal_dna_sequence *sequence,
                struct bsal_dna_codec *codec, struct bsal_memory_pool *memory)
{
    struct bsal_vector *entries;
    struct bsal_dna_sequence copy;

    entries = bsal_input_command_entries(self);

    bsal_dna_sequence_init_copy(&copy,
                sequence, codec, memory);
    bsal_vector_push_back(entries, &copy);
}

void bsal_input_command_init_empty(struct bsal_input_command *self)
{
    bsal_input_command_init(self, 0, 0, 0);
}
