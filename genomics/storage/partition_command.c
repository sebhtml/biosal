
#include "partition_command.h"

#include <core/system/packer.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <inttypes.h>

void biosal_partition_command_init(struct biosal_partition_command *self, int name,
                int stream_index, uint64_t stream_first, uint64_t stream_last,
                int store_index, uint64_t store_first, uint64_t store_last,
                uint64_t global_first, uint64_t global_last)
{
    self->name = name;

    self->stream_index = stream_index;
    self->stream_first = stream_first;
    self->stream_last = stream_last;

    self->store_index = store_index;
    self->store_first = store_first;
    self->store_last = store_last;

    self->global_first = global_first;
    self->global_last = global_last;
}

void biosal_partition_command_destroy(struct biosal_partition_command *self)
{
    self->name = -1;

    self->stream_index = -1;
    self->stream_first = 0;
    self->stream_last = 0;

    self->store_index = -1;
    self->store_first = 0;
    self->store_last = 0;

    self->global_first= 0;
    self->global_last = 0;
}

int biosal_partition_command_name(struct biosal_partition_command *self)
{
    return self->name;
}

int biosal_partition_command_stream_index(struct biosal_partition_command *self)
{
    return self->stream_index;
}

uint64_t biosal_partition_command_global_first(struct biosal_partition_command *self)
{
    return self->global_first;
}

uint64_t biosal_partition_command_global_last(struct biosal_partition_command *self)
{
    return self->global_last;
}

uint64_t biosal_partition_command_stream_first(struct biosal_partition_command *self)
{
    return self->stream_first;
}

uint64_t biosal_partition_command_stream_last(struct biosal_partition_command *self)
{
    return self->stream_last;
}

int biosal_partition_command_store_index(struct biosal_partition_command *self)
{
    return self->store_index;
}

int biosal_partition_command_pack_size(struct biosal_partition_command *self)
{
    return biosal_partition_command_pack_unpack(self, NULL, BIOSAL_PACKER_OPERATION_PACK_SIZE);
}

void biosal_partition_command_pack(struct biosal_partition_command *self, void *buffer)
{
    biosal_partition_command_pack_unpack(self, buffer, BIOSAL_PACKER_OPERATION_PACK);
}

void biosal_partition_command_unpack(struct biosal_partition_command *self, void *buffer)
{
#ifdef BIOSAL_PARTITION_COMMAND_DEBUG
    printf("DEBUG biosal_partition_command_unpack \n");
#endif

    biosal_partition_command_pack_unpack(self, buffer, BIOSAL_PACKER_OPERATION_UNPACK);
}

void biosal_partition_command_print(struct biosal_partition_command *self)
{
    printf(">> partition command # %d"
                    ", stream %d range %" PRIu64 "-%" PRIu64 " (%" PRIu64 ")"
                    ", store %d range %" PRIu64 "-%" PRIu64 " (%" PRIu64 ")"
                    ", global range %" PRIu64 "-%" PRIu64 "\n",
                    self->name,

                    self->stream_index, self->stream_first, self->stream_last,
                    self->stream_last - self->stream_first + 1,

                    self->store_index, self->store_first, self->store_last,
                    self->store_last - self->store_first + 1,

                    self->global_first, self->global_last);
}

uint64_t biosal_partition_command_store_first(struct biosal_partition_command *self)
{
    return self->store_first;
}

uint64_t biosal_partition_command_store_last(struct biosal_partition_command *self)
{
    return self->store_last;
}

int biosal_partition_command_pack_unpack(struct biosal_partition_command *self, void *buffer,
                int operation)
{
    struct biosal_packer packer;
    int bytes;

#ifdef BIOSAL_PARTITION_COMMAND_DEBUG
    printf("DEBUG biosal_partition_command_pack_unpack operation %d\n",
                    operation);
#endif

    biosal_packer_init(&packer, operation, buffer);

    biosal_packer_process(&packer, &self->name, sizeof(self->name));

    biosal_packer_process(&packer, &self->stream_index, sizeof(self->stream_index));
    biosal_packer_process(&packer, &self->stream_first, sizeof(self->stream_first));
    biosal_packer_process(&packer, &self->stream_last, sizeof(self->stream_last));

    biosal_packer_process(&packer, &self->store_index, sizeof(self->store_index));
    biosal_packer_process(&packer, &self->store_first, sizeof(self->store_first));
    biosal_packer_process(&packer, &self->store_last, sizeof(self->store_last));

    biosal_packer_process(&packer, &self->global_first, sizeof(self->global_first));
    biosal_packer_process(&packer, &self->global_last, sizeof(self->global_last));

    bytes = biosal_packer_get_byte_count(&packer);

    biosal_packer_destroy(&packer);

    return bytes;
}
