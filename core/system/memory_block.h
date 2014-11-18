
#ifndef CORE_MEMORY_BLOCK_H
#define CORE_MEMORY_BLOCK_H

/*
 * Memory block for the memory pool.
 */
struct core_memory_block {
    void *memory;
    int total_bytes;
    int offset;
    int name;
};

void core_memory_block_init(struct core_memory_block *self, int total_bytes,
                int name);
void core_memory_block_destroy(struct core_memory_block *self);
void *core_memory_block_allocate(struct core_memory_block *self, int size);
void core_memory_block_free(struct core_memory_block *self, void *pointer);
void core_memory_block_free_all(struct core_memory_block *self);

#endif
