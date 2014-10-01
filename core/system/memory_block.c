
#include "memory_block.h"

#include <core/system/memory.h>

#include <stdlib.h>

#define MEMORY_BLOCK 0x146f7d15

void biosal_memory_block_init(struct biosal_memory_block *self, int total_bytes)
{
    self->total_bytes = total_bytes;
    self->offset = 0;
    self->memory = NULL;
}

void biosal_memory_block_destroy(struct biosal_memory_block *self)
{
    self->total_bytes = 0;
    self->offset = 0;

    if (self->memory != NULL) {
        biosal_memory_free(self->memory, MEMORY_BLOCK);
        self->memory = NULL;
    }
}

void *biosal_memory_block_allocate(struct biosal_memory_block *self, int size)
{
    void *pointer;

    if (self->memory == NULL) {
        self->memory = biosal_memory_allocate(self->total_bytes, MEMORY_BLOCK);
    }

    if (self->offset + size > self->total_bytes) {
        return NULL;
    }

    pointer = ((char *)self->memory) + self->offset;
    self->offset += size;

    return pointer;
}

void biosal_memory_block_free(struct biosal_memory_block *self, void *pointer)
{
    /* do nothing */
}

void biosal_memory_block_free_all(struct biosal_memory_block *self)
{
    /* constant-time massive deallocation of memory blocks
     */
    self->offset = 0;
}
