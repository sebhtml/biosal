
#include "stream_command.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <inttypes.h>

void bsal_stream_command_init(struct bsal_stream_command *self, int name,
                int stream_index, int first, int last,
                uint64_t global_first, uint64_t global_last, int store_index)
{
    self->name = name;
    self->stream_index = stream_index;

    self->first = first;
    self->last = last;
    self->global_first = global_first;
    self->global_last = global_last;

    self->store_index = store_index;
}

void bsal_stream_command_destroy(struct bsal_stream_command *self)
{
    self->name = -1;
    self->stream_index = -1;

    self->first = -1;
    self->last = -1;
    self->global_first= -1;
    self->global_last = -1;

    self->store_index = -1;
}

int bsal_stream_command_name(struct bsal_stream_command *self)
{
    return self->name;
}

int bsal_stream_command_stream_index(struct bsal_stream_command *self)
{
    return self->stream_index;
}

uint64_t bsal_stream_command_global_first(struct bsal_stream_command *self)
{
    return self->first;
}

uint64_t bsal_stream_command_global_last(struct bsal_stream_command *self)
{
    return self->last;
}

int bsal_stream_command_first(struct bsal_stream_command *self)
{
    return self->first;
}

int bsal_stream_command_last(struct bsal_stream_command *self)
{
    return self->last;
}

int bsal_stream_command_store_index(struct bsal_stream_command *self)
{
    return self->store_index;
}

int bsal_stream_command_pack_size(struct bsal_stream_command *self)
{
    return 5 * sizeof(int) + 2 * sizeof(uint64_t);
}

void bsal_stream_command_pack(struct bsal_stream_command *self, void *buffer)
{
    int offset;
    int bytes;

    if (buffer == NULL) {
        return;
    }

    offset = 0;

    bytes = sizeof(self->name);
    memcpy((char *)buffer + offset, &self->name, bytes);
    offset += bytes;
    memcpy((char *)buffer + offset, &self->stream_index, bytes);
    offset += bytes;

    memcpy((char *)buffer + offset, &self->first, bytes);
    offset += bytes;
    memcpy((char *)buffer + offset, &self->last, bytes);
    offset += bytes;

    bytes = sizeof(self->global_first);
    memcpy((char *)buffer + offset, &self->global_first, bytes);
    offset += bytes;
    memcpy((char *)buffer + offset, &self->global_last, bytes);
    offset += bytes;

    bytes = sizeof(self->store_index);
    memcpy((char *)buffer + offset, &self->store_index, bytes);
    offset += bytes;
}

void bsal_stream_command_unpack(struct bsal_stream_command *self, void *buffer)
{
    int offset;
    int bytes;

    if (buffer == NULL) {
        return;
    }

    offset = 0;

    bytes = sizeof(self->name);
    memcpy(&self->name, (char *)buffer + offset, bytes);
    offset += bytes;
    memcpy(&self->stream_index, (char *)buffer + offset, bytes);
    offset += bytes;

    memcpy(&self->first, (char *)buffer + offset, bytes);
    offset += bytes;
    memcpy(&self->last, (char *)buffer + offset, bytes);
    offset += bytes;

    bytes = sizeof(self->global_first);
    memcpy(&self->global_first, (char *)buffer + offset, bytes);
    offset += bytes;
    memcpy(&self->global_last, (char *)buffer + offset, bytes);
    offset += bytes;

    bytes = sizeof(self->store_index);
    memcpy(&self->store_index, (char *)buffer + offset, bytes);
    offset += bytes;
}

void bsal_stream_command_print(struct bsal_stream_command *self)
{
    printf(">> stream command # %d, stream %d from %d to %d (%d), destination %d - global range: %" PRIu64 "-%" PRIu64 "\n",
                    self->name,
                    self->stream_index, self->first,
                    self->last, self->last - self->first + 1,
                    self->store_index,
                    self->global_first, self->global_last);
}
