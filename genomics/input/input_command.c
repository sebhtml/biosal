
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

    biosal_vector_init(&self->entries, sizeof(struct biosal_dna_sequence));
}

void biosal_input_command_destroy(struct biosal_input_command *self, struct biosal_memory_pool *memory)
{
    struct biosal_dna_sequence *sequence;
    struct biosal_vector_iterator iterator;
    self->store_name= -1;
    self->store_first = 0;
    self->store_last = 0;

    biosal_vector_iterator_init(&iterator, &self->entries);

    while (biosal_vector_iterator_has_next(&iterator)) {
        biosal_vector_iterator_next(&iterator, (void **)&sequence);

        biosal_dna_sequence_destroy(sequence, memory);
    }

    biosal_vector_iterator_destroy(&iterator);

    biosal_vector_destroy(&self->entries);
}

int biosal_input_command_store_name(struct biosal_input_command *self)
{
    return self->store_name;
}

int biosal_input_command_pack_size(struct biosal_input_command *self,
                struct biosal_dna_codec *codec)
{
    return biosal_input_command_pack_unpack(self, NULL, BIOSAL_PACKER_OPERATION_PACK_SIZE, NULL, codec);
}

int biosal_input_command_pack(struct biosal_input_command *self, void *buffer,
                struct biosal_dna_codec *codec)
{
    return biosal_input_command_pack_unpack(self, buffer, BIOSAL_PACKER_OPERATION_PACK, NULL, codec);
}

int biosal_input_command_unpack(struct biosal_input_command *self, void *buffer, struct biosal_memory_pool *memory,
                struct biosal_dna_codec *codec)
{
#ifdef BIOSAL_INPUT_COMMAND_DEBUG
    printf("DEBUG biosal_input_command_unpack %p\n", buffer);
#endif

    return biosal_input_command_pack_unpack(self, buffer, BIOSAL_PACKER_OPERATION_UNPACK, memory, codec);
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
                int operation, struct biosal_memory_pool *memory, struct biosal_dna_codec *codec)
{
    struct biosal_packer packer;
    int offset;
    int64_t entries;
    struct biosal_dna_sequence dna_sequence;
    struct biosal_dna_sequence *other_sequence;
    int bytes;
    int i;

#ifdef BIOSAL_INPUT_COMMAND_DEBUG
    if (1 || operation == BIOSAL_PACKER_OPERATION_UNPACK) {
        printf("DEBUG ENTRY biosal_input_command_pack_unpack operation %d\n",
                        operation);
    }
#endif

    offset = 0;

    biosal_packer_init(&packer, operation, buffer);

    biosal_packer_process(&packer, &self->store_name, sizeof(self->store_name));
    biosal_packer_process(&packer, &self->store_first, sizeof(self->store_first));
    biosal_packer_process(&packer, &self->store_last, sizeof(self->store_last));

    /* TODO remove this line
     */

    /*
    if (operation == BIOSAL_PACKER_OPERATION_UNPACK) {
        biosal_vector_init(&self->entries, sizeof(struct biosal_dna_sequence));
    }

    return biosal_packer_get_byte_count(&packer);
*/

#ifdef BIOSAL_INPUT_COMMAND_DEBUG
    if (1 || operation == BIOSAL_PACKER_OPERATION_UNPACK) {
        printf("DEBUG biosal_input_command_pack_unpack offset after packing stuff %d\n",
                    offset);
    }
#endif

    /* do the vector too
     * process the count.
     * then, process every read.
     */

    if (operation == BIOSAL_PACKER_OPERATION_PACK
                    || operation == BIOSAL_PACKER_OPERATION_PACK_SIZE) {
        entries = biosal_vector_size(&self->entries);
    } else if (operation == BIOSAL_PACKER_OPERATION_UNPACK) {

        biosal_vector_init(&self->entries, sizeof(struct biosal_dna_sequence));

        biosal_vector_set_memory_pool(&self->entries, memory);
    }

    /* 1. entries
     */
    biosal_packer_process(&packer, &entries, sizeof(entries));

    offset = biosal_packer_get_byte_count(&packer);
    biosal_packer_destroy(&packer);

#ifdef BIOSAL_INPUT_COMMAND_DEBUG
    printf("DEBUG biosal_input_command_pack_unpack operation %d entries %d offset %d\n",
                    operation, entries, offset);
#endif

    /* 2. get the entries
     */
    if (operation == BIOSAL_PACKER_OPERATION_UNPACK) {

        while (entries--) {
            bytes = biosal_dna_sequence_unpack(&dna_sequence,
                            (char *) buffer + offset,
                            memory, codec);

#ifdef BIOSAL_INPUT_COMMAND_DEBUG
            printf("DEBUG unpacking DNA sequence, used %d bytes\n",
                            bytes);
#endif
            offset += bytes;

            biosal_vector_push_back(&self->entries, &dna_sequence);
        }
    } else if (operation == BIOSAL_PACKER_OPERATION_PACK) {

        i = 0;
        while (entries--) {
            other_sequence = (struct biosal_dna_sequence *)biosal_vector_at(&self->entries,
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
    } else if (operation == BIOSAL_PACKER_OPERATION_PACK_SIZE) {

        i = 0;
        while (entries--) {
            other_sequence = (struct biosal_dna_sequence *)biosal_vector_at(&self->entries,
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

    if (operation == BIOSAL_PACKER_OPERATION_UNPACK) {
        printf("DEBUG biosal_input_command_pack_unpack unpacked %d entries\n",
                        (int)biosal_vector_size(&self->entries));
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
            (int)biosal_vector_size(&self->entries), biosal_input_command_pack_size(self, codec));
}

struct biosal_vector *biosal_input_command_entries(struct biosal_input_command *self)
{
    return &self->entries;
}

int biosal_input_command_entry_count(struct biosal_input_command *self)
{
    return biosal_vector_size(&self->entries);
}

void biosal_input_command_add_entry(struct biosal_input_command *self,
                struct biosal_dna_sequence *sequence,
                struct biosal_dna_codec *codec, struct biosal_memory_pool *memory)
{
    struct biosal_vector *entries;
    struct biosal_dna_sequence copy;

    entries = biosal_input_command_entries(self);

    biosal_dna_sequence_init_copy(&copy,
                sequence, codec, memory);
    biosal_vector_push_back(entries, &copy);
}

void biosal_input_command_init_empty(struct biosal_input_command *self)
{
    biosal_input_command_init(self, 0, 0, 0);
}
