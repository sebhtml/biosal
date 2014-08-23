
#ifndef BSAL_MEMORY_POOL_H
#define BSAL_MEMORY_POOL_H

#include "memory.h"
#include "memory_block.h"

#include <core/structures/map.h>
#include <core/structures/set.h>
#include <core/structures/queue.h>

#include <stdint.h>

/*
*/

/*
 * Disable particular memory pool on IBM
 * Blue Gene/Q because otherwise it does not
 * work.
 */
#if defined(__bgq__)
#define BSAL_MEMORY_POOL_DISABLE_MESSAGE_BUFFER_POOL
#endif

/*
 * \see https://wiki.alcf.anl.gov/parts/index.php/Blue_Gene/Q
 * \see https://svn.mcs.anl.gov/repos/ZeptoOS/trunk/BGP/comm/arch-runtime/arch/include/cnk/vmm.h
 * \see https://github.com/jedbrown/bgq-driver/blob/master/cnk/include/Config.h
 */

#if defined(__bgq__)
#define BSAL_MEMORY_POOL_MESSAGE_BUFFER_BLOCK_SIZE (4 * 1024)
#else
#define BSAL_MEMORY_POOL_MESSAGE_BUFFER_BLOCK_SIZE (2 * 1024 * 1024)
#endif

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

    uint32_t flags;
    int block_size;
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

void bsal_memory_pool_disable_normalization(struct bsal_memory_pool *self);
void bsal_memory_pool_enable_normalization(struct bsal_memory_pool *self);

void bsal_memory_pool_disable_alignment(struct bsal_memory_pool *self);
void bsal_memory_pool_enable_alignment(struct bsal_memory_pool *self);

#endif
