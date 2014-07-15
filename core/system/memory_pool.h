
#ifndef BSAL_MEMORY_POOL_H
#define BSAL_MEMORY_POOL_H

#include "memory.h"
#include "memory_block.h"

#include <core/structures/map.h>
#include <core/structures/set.h>
#include <core/structures/queue.h>

/*
 * \see http://en.wikipedia.org/wiki/Memory_pool
 */
struct bsal_memory_pool {
    struct bsal_map recycle_bin;
    struct bsal_map allocated_blocks;
    struct bsal_set large_blocks;
    struct bsal_memory_block *current_block;
    struct bsal_queue ready_blocks;
    struct bsal_queue dried_blocks;
    int block_size;
    int tracking_is_enabled;
    int disabled;
};

void bsal_memory_pool_init(struct bsal_memory_pool *self, int block_size);
void bsal_memory_pool_destroy(struct bsal_memory_pool *self);
void *bsal_memory_pool_allocate(struct bsal_memory_pool *self, size_t size);
void bsal_memory_pool_free(struct bsal_memory_pool *self, void *pointer);

void bsal_memory_pool_enable_tracking(struct bsal_memory_pool *self);
void bsal_memory_pool_disable_tracking(struct bsal_memory_pool *self);

void bsal_memory_pool_free_all(struct bsal_memory_pool *self);
void bsal_memory_pool_disable(struct bsal_memory_pool *self);

void bsal_memory_pool_add_block(struct bsal_memory_pool *self);
void *bsal_memory_pool_allocate_private(struct bsal_memory_pool *self, size_t size);

#endif
