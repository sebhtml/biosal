
#ifndef BIOSAL_PARTITION_COMMAND_H
#define BIOSAL_PARTITION_COMMAND_H

#include <stdint.h>

/* This structure contains information
 * required to perform a stream operation
 */
struct biosal_partition_command {
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

void biosal_partition_command_init(struct biosal_partition_command *self, int name,
                int stream_index, uint64_t stream_first, uint64_t stream_last,
                int store_index, uint64_t store_first, uint64_t store_last,
                uint64_t global_first, uint64_t global_last);

void biosal_partition_command_destroy(struct biosal_partition_command *self);

int biosal_partition_command_name(struct biosal_partition_command *self);

int biosal_partition_command_stream_index(struct biosal_partition_command *self);
uint64_t biosal_partition_command_stream_first(struct biosal_partition_command *self);
uint64_t biosal_partition_command_stream_last(struct biosal_partition_command *self);

uint64_t biosal_partition_command_global_first(struct biosal_partition_command *self);
uint64_t biosal_partition_command_global_last(struct biosal_partition_command *self);

int biosal_partition_command_store_index(struct biosal_partition_command *self);
uint64_t biosal_partition_command_store_first(struct biosal_partition_command *self);
uint64_t biosal_partition_command_store_last(struct biosal_partition_command *self);

int biosal_partition_command_pack_size(struct biosal_partition_command *self);
void biosal_partition_command_pack(struct biosal_partition_command *self, void *buffer);
void biosal_partition_command_unpack(struct biosal_partition_command *self, void *buffer);

void biosal_partition_command_print(struct biosal_partition_command *self);
int biosal_partition_command_pack_unpack(struct biosal_partition_command *self, void *buffer,
                int operation);

#endif
