
#include "mega_block.h"

#include <stdio.h>
#include <inttypes.h>

void bsal_mega_block_init(struct bsal_mega_block *self, int file, uint64_t offset, uint64_t entries,
                uint64_t entries_from_start)
{
    self->file = file;
    self->offset = offset;
    self->entries = entries;
    self->entries_from_start = entries_from_start;

#if 0
    bsal_mega_block_print(self);
#endif
}

void bsal_mega_block_destroy(struct bsal_mega_block *self)
{
    self->file = 0;
    self->offset = 0;
    self->entries = 0;
}

void bsal_mega_block_print(struct bsal_mega_block *self)
{
    printf("MEGA BLOCK file %d offset %" PRIu64 " entries %" PRIu64 " %" PRIu64 "\n", self->file,
                    self->offset, self->entries, self->entries_from_start);
}

uint64_t bsal_mega_block_get_entries_from_start(struct bsal_mega_block *self)
{
    return self->entries_from_start;
}

uint64_t bsal_mega_block_get_offset(struct bsal_mega_block *self)
{
    return self->offset;
}

int bsal_mega_block_get_file(struct bsal_mega_block *self)
{
    return self->file;
}

uint64_t bsal_mega_block_get_entries(struct bsal_mega_block *self)
{
    return self->entries;
}

void bsal_mega_block_set_file(struct bsal_mega_block *block, int file)
{
    block->file = file;
}

void bsal_mega_block_set_entries_from_start(struct bsal_mega_block *self,
                uint64_t entries)
{
    self->entries_from_start = entries;
}
