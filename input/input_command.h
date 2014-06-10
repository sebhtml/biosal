
#ifndef BSAL_INPUT_COMMAND_H
#define BSAL_INPUT_COMMAND_H

#include <stdint.h>

#include <structures/vector.h>

/* This structure contains information
 * required to perform a stream operation
 */
struct bsal_input_command {
    int store_name;
    uint64_t store_first;
    uint64_t store_last;
    struct bsal_vector entries;
};

void bsal_input_command_init(struct bsal_input_command *self, int store_name,
                uint64_t store_first, uint64_t store_last);

void bsal_input_command_destroy(struct bsal_input_command *self);

int bsal_input_command_store_name(struct bsal_input_command *self);
uint64_t bsal_input_command_store_first(struct bsal_input_command *self);
uint64_t bsal_input_command_store_last(struct bsal_input_command *self);

int bsal_input_command_pack_size(struct bsal_input_command *self);
void bsal_input_command_pack(struct bsal_input_command *self, void *buffer);
void bsal_input_command_unpack(struct bsal_input_command *self, void *buffer);

int bsal_input_command_pack_unpack(struct bsal_input_command *self, void *buffer,
                int operation);
void bsal_input_command_print(struct bsal_input_command *self);

struct bsal_vector *bsal_input_command_entries(struct bsal_input_command *self);

#endif
