
#include "stream_command.h"

#include <system/packer.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <inttypes.h>

void bsal_stream_command_init(struct bsal_stream_command *self, int name,
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

void bsal_stream_command_destroy(struct bsal_stream_command *self)
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
    return self->global_first;
}

uint64_t bsal_stream_command_global_last(struct bsal_stream_command *self)
{
    return self->global_last;
}

uint64_t bsal_stream_command_stream_first(struct bsal_stream_command *self)
{
    return self->stream_first;
}

uint64_t bsal_stream_command_last(struct bsal_stream_command *self)
{
    return self->stream_last;
}

int bsal_stream_command_store_index(struct bsal_stream_command *self)
{
    return self->store_index;
}

int bsal_stream_command_pack_size(struct bsal_stream_command *self)
{
    return bsal_stream_command_pack_unpack(self, NULL, BSAL_PACKER_OPERATION_DRY_RUN);
}

void bsal_stream_command_pack(struct bsal_stream_command *self, void *buffer)
{
    bsal_stream_command_pack_unpack(self, buffer, BSAL_PACKER_OPERATION_PACK);
}

void bsal_stream_command_unpack(struct bsal_stream_command *self, void *buffer)
{
#ifdef BSAL_STREAM_COMMAND_DEBUG
    printf("DEBUG bsal_stream_command_unpack \n");
#endif

    bsal_stream_command_pack_unpack(self, buffer, BSAL_PACKER_OPERATION_UNPACK);
}

void bsal_stream_command_print(struct bsal_stream_command *self)
{
    printf(">> stream command # %d"
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

uint64_t bsal_stream_command_store_first(struct bsal_stream_command *self)
{
    return self->store_first;
}

uint64_t bsal_stream_command_store_last(struct bsal_stream_command *self)
{
    return self->store_last;
}

int bsal_stream_command_pack_unpack(struct bsal_stream_command *self, void *buffer,
                int operation)
{
    struct bsal_packer packer;
    int bytes;

#ifdef BSAL_STREAM_COMMAND_DEBUG
    printf("DEBUG bsal_stream_command_pack_unpack operation %d\n",
                    operation);
#endif

    bsal_packer_init(&packer, operation, buffer);

    bsal_packer_work(&packer, &self->name, sizeof(self->name));

    bsal_packer_work(&packer, &self->stream_index, sizeof(self->stream_index));
    bsal_packer_work(&packer, &self->stream_first, sizeof(self->stream_first));
    bsal_packer_work(&packer, &self->stream_last, sizeof(self->stream_last));

    bsal_packer_work(&packer, &self->store_index, sizeof(self->store_index));
    bsal_packer_work(&packer, &self->store_first, sizeof(self->store_first));
    bsal_packer_work(&packer, &self->store_last, sizeof(self->store_last));

    bsal_packer_work(&packer, &self->global_first, sizeof(self->global_first));
    bsal_packer_work(&packer, &self->global_last, sizeof(self->global_last));

    bytes = bsal_packer_worked_bytes(&packer);

    bsal_packer_destroy(&packer);

    return bytes;
}
