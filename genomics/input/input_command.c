
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
#define BIOSAL_INPUT_COMMAND_DEBUG
*/

void biosal_input_command_init(struct biosal_input_command *self,
                int store_name, uint64_t store_first, uint64_t store_last)
{
    self->store_name = store_name;
    self->store_first = store_first;
    self->store_last = store_last;

    core_vector_init(&self->entries, sizeof(struct biosal_dna_sequence));
}

void biosal_input_command_destroy(struct biosal_input_command *self, struct core_memory_pool *memory)
{
    struct biosal_dna_sequence *sequence;
    struct core_vector_iterator iterator;
    self->store_name= -1;
    self->store_first = 0;
    self->store_last = 0;

    core_vector_iterator_init(&iterator, &self->entries);

    while (core_vector_iterator_has_next(&iterator)) {
        core_vector_iterator_next(&iterator, (void **)&sequence);

        biosal_dna_sequence_destroy(sequence, memory);
    }

    core_vector_iterator_destroy(&iterator);

    core_vector_destroy(&self->entries);
}

int biosal_input_command_store_name(struct biosal_input_command *self)
{
    return self->store_name;
}

int biosal_input_command_pack_size(struct biosal_input_command *self,
                struct biosal_dna_codec *codec)
{
    return biosal_input_command_pack_unpack(self, NULL, CORE_PACKER_OPERATION_PACK_SIZE, NULL, codec);
}

int biosal_input_command_pack(struct biosal_input_command *self, void *buffer,
                struct biosal_dna_codec *codec)
{
    return biosal_input_command_pack_unpack(self, buffer, CORE_PACKER_OPERATION_PACK, NULL, codec);
}

int biosal_input_command_unpack(struct biosal_input_command *self, void *buffer, struct core_memory_pool *memory,
                struct biosal_dna_codec *codec)
{
#ifdef BIOSAL_INPUT_COMMAND_DEBUG
    printf("DEBUG biosal_input_command_unpack %p\n", buffer);
#endif

    return biosal_input_command_pack_unpack(self, buffer, CORE_PACKER_OPERATION_UNPACK, memory, codec);
}

uint64_t biosal_input_command_store_first(struct biosal_input_command *self)
{
    return self->store_first;
}

uint64_t biosal_input_command_store_last(struct biosal_input_command *self)
{
    return self->store_last;
}

int biosal_input_command_pack_unpack(struct biosal_input_command *self, void *buffer,
                int operation, struct core_memory_pool *memory, struct biosal_dna_codec *codec)
{
    struct core_packer packer;
    int offset;
    int64_t entries;
    struct biosal_dna_sequence dna_sequence;
    struct biosal_dna_sequence *other_sequence;
    int bytes;
    int i;

#ifdef BIOSAL_INPUT_COMMAND_DEBUG
    if (1 || operation == CORE_PACKER_OPERATION_UNPACK) {
        printf("DEBUG ENTRY biosal_input_command_pack_unpack operation %d\n",
                        operation);
    }
#endif

    offset = 0;

    core_packer_init(&packer, operation, buffer);

    core_packer_process(&packer, &self->store_name, sizeof(self->store_name));
    core_packer_process(&packer, &self->store_first, sizeof(self->store_first));
    core_packer_process(&packer, &self->store_last, sizeof(self->store_last));

    /* TODO remove this line
     */

    /*
    if (operation == CORE_PACKER_OPERATION_UNPACK) {
        core_vector_init(&self->entries, sizeof(struct biosal_dna_sequence));
    }

    return core_packer_get_byte_count(&packer);
*/

#ifdef BIOSAL_INPUT_COMMAND_DEBUG
    if (1 || operation == CORE_PACKER_OPERATION_UNPACK) {
        printf("DEBUG biosal_input_command_pack_unpack offset after packing stuff %d\n",
                    offset);
    }
#endif

    /* do the vector too
     * process the count.
     * then, process every read.
     */

    if (operation == CORE_PACKER_OPERATION_PACK
                    || operation == CORE_PACKER_OPERATION_PACK_SIZE) {
        entries = core_vector_size(&self->entries);
    } else if (operation == CORE_PACKER_OPERATION_UNPACK) {

        core_vector_init(&self->entries, sizeof(struct biosal_dna_sequence));

        core_vector_set_memory_pool(&self->entries, memory);
    }

    /* 1. entries
     */
    core_packer_process(&packer, &entries, sizeof(entries));

    offset = core_packer_get_byte_count(&packer);
    core_packer_destroy(&packer);

#ifdef BIOSAL_INPUT_COMMAND_DEBUG
    printf("DEBUG biosal_input_command_pack_unpack operation %d entries %d offset %d\n",
                    operation, entries, offset);
#endif

    /* 2. get the entries
     */
    if (operation == CORE_PACKER_OPERATION_UNPACK) {

        while (entries--) {
            bytes = biosal_dna_sequence_unpack(&dna_sequence,
                            (char *) buffer + offset,
                            memory, codec);

#ifdef BIOSAL_INPUT_COMMAND_DEBUG
            printf("DEBUG unpacking DNA sequence, used %d bytes\n",
                            bytes);
#endif
            offset += bytes;

            core_vector_push_back(&self->entries, &dna_sequence);
        }
    } else if (operation == CORE_PACKER_OPERATION_PACK) {

        i = 0;
        while (entries--) {
            other_sequence = (struct biosal_dna_sequence *)core_vector_at(&self->entries,
                            i++);
            bytes = biosal_dna_sequence_pack(other_sequence,
                            (char *) buffer + offset,
                            codec);
            offset += bytes;

#ifdef BIOSAL_INPUT_COMMAND_DEBUG
            printf("DEBUG packing DNA sequence, used %d bytes, sum %d\n",
                            bytes, offset);
#endif
        }
    } else if (operation == CORE_PACKER_OPERATION_PACK_SIZE) {

        i = 0;
        while (entries--) {
            other_sequence = (struct biosal_dna_sequence *)core_vector_at(&self->entries,
                            i++);
            bytes = biosal_dna_sequence_pack_size(other_sequence, codec);

            offset += bytes;

#ifdef BIOSAL_INPUT_COMMAND_DEBUG
            printf("DEBUG dry run DNA sequence, used %d bytes, sum %d\n",
                            bytes, offset);
#endif
        }
    }

#ifdef BIOSAL_INPUT_COMMAND_DEBUG
    printf("DEBUG biosal_input_command_pack_unpack operation %d final offset %d\n",
                    operation, offset);

    if (operation == CORE_PACKER_OPERATION_UNPACK) {
        printf("DEBUG biosal_input_command_pack_unpack unpacked %d entries\n",
                        (int)core_vector_size(&self->entries));
    }
#endif

    return offset;
}

void biosal_input_command_print(struct biosal_input_command *self,
                struct biosal_dna_codec *codec)
{
    printf("[===] input command: store_name %d store_first %" PRIu64 " store_last %" PRIu64 ""
                    " entries %d bytes %d\n",
            self->store_name, self->store_first, self->store_last,
            (int)core_vector_size(&self->entries), biosal_input_command_pack_size(self, codec));
}

struct core_vector *biosal_input_command_entries(struct biosal_input_command *self)
{
    return &self->entries;
}

int biosal_input_command_entry_count(struct biosal_input_command *self)
{
    return core_vector_size(&self->entries);
}

void biosal_input_command_add_entry(struct biosal_input_command *self,
                struct biosal_dna_sequence *sequence,
                struct biosal_dna_codec *codec, struct core_memory_pool *memory)
{
    struct core_vector *entries;
    struct biosal_dna_sequence copy;

    entries = biosal_input_command_entries(self);

    biosal_dna_sequence_init_copy(&copy,
                sequence, codec, memory);
    core_vector_push_back(entries, &copy);
}

void biosal_input_command_init_empty(struct biosal_input_command *self)
{
    biosal_input_command_init(self, 0, 0, 0);
}
