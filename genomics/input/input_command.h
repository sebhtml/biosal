
#ifndef BIOSAL_INPUT_COMMAND_H
#define BIOSAL_INPUT_COMMAND_H

#include <stdint.h>

#include <genomics/data/dna_codec.h>
#include <genomics/data/dna_sequence.h>

#include <core/structures/vector.h>

/* This structure contains information
 * required to perform a stream operation
 */
struct biosal_input_command {
    int store_name;
    uint64_t store_first;
    uint64_t store_last;
    struct core_vector entries;
};

void biosal_input_command_init(struct biosal_input_command *self, int store_name,
                uint64_t store_first, uint64_t store_last);
void biosal_input_command_init_empty(struct biosal_input_command *self);

void biosal_input_command_destroy(struct biosal_input_command *self, struct core_memory_pool *memory);

int biosal_input_command_store_name(struct biosal_input_command *self);
uint64_t biosal_input_command_store_first(struct biosal_input_command *self);
uint64_t biosal_input_command_store_last(struct biosal_input_command *self);

int biosal_input_command_pack_size(struct biosal_input_command *self, struct biosal_dna_codec *codec);
int biosal_input_command_pack(struct biosal_input_command *self, void *buffer, struct biosal_dna_codec *codec);
int biosal_input_command_unpack(struct biosal_input_command *self, void *buffer,
                struct core_memory_pool *memory, struct biosal_dna_codec *codec);

int biosal_input_command_pack_unpack(struct biosal_input_command *self, void *buffer,
                int operation, struct core_memory_pool *memory,
                struct biosal_dna_codec *codec);
void biosal_input_command_print(struct biosal_input_command *self, struct biosal_dna_codec *codec);

struct core_vector *biosal_input_command_entries(struct biosal_input_command *self);
int biosal_input_command_entry_count(struct biosal_input_command *self);
void biosal_input_command_add_entry(struct biosal_input_command *self,
                struct biosal_dna_sequence *sequence,
                struct biosal_dna_codec *codec, struct core_memory_pool *memory);

#endif
