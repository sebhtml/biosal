
#ifndef BSAL_PARTITION_COMMAND_H
#define BSAL_PARTITION_COMMAND_H

#include <stdint.h>

/* This structure contains information
 * required to perform a stream operation
 */
struct bsal_partition_command {
    int name;

    int stream_index;
    uint64_t stream_first;
    uint64_t stream_last;

    int store_index;
    uint64_t store_first;
    uint64_t store_last;

    uint64_t global_first;
    uint64_t global_last;
};

void bsal_partition_command_init(struct bsal_partition_command *self, int name,
                int stream_index, uint64_t stream_first, uint64_t stream_last,
                int store_index, uint64_t store_first, uint64_t store_last,
                uint64_t global_first, uint64_t global_last);

void bsal_partition_command_destroy(struct bsal_partition_command *self);

int bsal_partition_command_name(struct bsal_partition_command *self);

int bsal_partition_command_stream_index(struct bsal_partition_command *self);
uint64_t bsal_partition_command_stream_first(struct bsal_partition_command *self);
uint64_t bsal_partition_command_stream_last(struct bsal_partition_command *self);

uint64_t bsal_partition_command_global_first(struct bsal_partition_command *self);
uint64_t bsal_partition_command_global_last(struct bsal_partition_command *self);

int bsal_partition_command_store_index(struct bsal_partition_command *self);
uint64_t bsal_partition_command_store_first(struct bsal_partition_command *self);
uint64_t bsal_partition_command_store_last(struct bsal_partition_command *self);

int bsal_partition_command_pack_size(struct bsal_partition_command *self);
void bsal_partition_command_pack(struct bsal_partition_command *self, void *buffer);
void bsal_partition_command_unpack(struct bsal_partition_command *self, void *buffer);

void bsal_partition_command_print(struct bsal_partition_command *self);
int bsal_partition_command_pack_unpack(struct bsal_partition_command *self, void *buffer,
                int operation);

#endif
