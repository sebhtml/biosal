
#ifndef BIOSAL_MEMORY_BLOCK_H
#define BIOSAL_MEMORY_BLOCK_H

struct biosal_memory_block {
    void *memory;
    int total_bytes;
    int offset;
};

void biosal_memory_block_init(struct biosal_memory_block *self, int total_bytes);
void biosal_memory_block_destroy(struct biosal_memory_block *self);
void *biosal_memory_block_allocate(struct biosal_memory_block *self, int size);
void biosal_memory_block_free(struct biosal_memory_block *self, void *pointer);
void biosal_memory_block_free_all(struct biosal_memory_block *self);

#endif
