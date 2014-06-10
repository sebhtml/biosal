
#ifndef BSAL_STREAM_COMMAND_H
#define BSAL_STREAM_COMMAND_H

#include <stdint.h>

struct bsal_stream_command {
    int name;
    int stream_index;
    int first;
    int last;
    uint64_t global_first;
    uint64_t global_last;
    int store_index;
};

void bsal_stream_command_init(struct bsal_stream_command *self, int name,
                int stream_index, int first, int last,
                uint64_t global_first, uint64_t global_last, int store_index);

void bsal_stream_command_destroy(struct bsal_stream_command *self);

int bsal_stream_command_name(struct bsal_stream_command *self);
int bsal_stream_command_stream_index(struct bsal_stream_command *self);

int bsal_stream_command_first(struct bsal_stream_command *self);
int bsal_stream_command_last(struct bsal_stream_command *self);
uint64_t bsal_stream_command_global_first(struct bsal_stream_command *self);
uint64_t bsal_stream_command_global_last(struct bsal_stream_command *self);

int bsal_stream_command_store_index(struct bsal_stream_command *self);

int bsal_stream_command_pack_size(struct bsal_stream_command *self);
void bsal_stream_command_pack(struct bsal_stream_command *self, void *buffer);
void bsal_stream_command_unpack(struct bsal_stream_command *self, void *buffer);

void bsal_stream_command_print(struct bsal_stream_command *self);

#endif
