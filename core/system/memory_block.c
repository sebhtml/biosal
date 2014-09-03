
#include "memory_block.h"

#include <core/system/memory.h>

#include <stdlib.h>

void bsal_memory_block_init(struct bsal_memory_block *self, int total_bytes)
{
    self->total_bytes = total_bytes;
    self->offset = 0;
    self->memory = NULL;
}

void bsal_memory_block_destroy(struct bsal_memory_block *self)
{
    self->total_bytes = 0;
    self->offset = 0;

    if (self->memory != NULL) {
        bsal_memory_free(self->memory);
        self->memory = NULL;
    }
}

void *bsal_memory_block_allocate(struct bsal_memory_block *self, int size)
{
    void *pointer;

    if (self->memory == NULL) {
        self->memory = bsal_memory_allocate(self->total_bytes);
    }

    if (self->offset + size > self->total_bytes) {
        return NULL;
    }

    pointer = ((char *)self->memory) + self->offset;
    self->offset += size;

    return pointer;
}

void bsal_memory_block_free(struct bsal_memory_block *self, void *pointer)
{
    /* do nothing */
}

void bsal_memory_block_free_all(struct bsal_memory_block *self)
{
    /* constant-time massive deallocation of memory blocks
     */
    self->offset = 0;
}
