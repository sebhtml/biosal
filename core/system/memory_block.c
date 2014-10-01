
#include "memory_block.h"

#include <core/system/memory.h>

#include <stdlib.h>

#define MEMORY_BLOCK 0x146f7d15

void core_memory_block_init(struct core_memory_block *self, int total_bytes)
{
    self->total_bytes = total_bytes;
    self->offset = 0;
    self->memory = NULL;
}

void core_memory_block_destroy(struct core_memory_block *self)
{
    self->total_bytes = 0;
    self->offset = 0;

    if (self->memory != NULL) {
        core_memory_free(self->memory, MEMORY_BLOCK);
        self->memory = NULL;
    }
}

void *core_memory_block_allocate(struct core_memory_block *self, int size)
{
    void *pointer;

    if (self->memory == NULL) {
        self->memory = core_memory_allocate(self->total_bytes, MEMORY_BLOCK);
    }

    if (self->offset + size > self->total_bytes) {
        return NULL;
    }

    pointer = ((char *)self->memory) + self->offset;
    self->offset += size;

    return pointer;
}

void core_memory_block_free(struct core_memory_block *self, void *pointer)
{
    /* do nothing */
}

void core_memory_block_free_all(struct core_memory_block *self)
{
    /* constant-time massive deallocation of memory blocks
     */
    self->offset = 0;
}
