
#ifndef BSAL_MEMORY_BLOCK_H
#define BSAL_MEMORY_BLOCK_H

struct bsal_memory_block {
    void *memory;
    int total_bytes;
    int offset;
};

void bsal_memory_block_init(struct bsal_memory_block *self, int total_bytes);
void bsal_memory_block_destroy(struct bsal_memory_block *self);
void *bsal_memory_block_allocate(struct bsal_memory_block *self, int size);
void bsal_memory_block_free(struct bsal_memory_block *self, void *pointer);
void bsal_memory_block_free_all(struct bsal_memory_block *self);

#endif
