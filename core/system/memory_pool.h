
#ifndef CORE_MEMORY_POOL_H
#define CORE_MEMORY_POOL_H

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
#if 0
#if defined(__bgq__)
#define CORE_MEMORY_POOL_DISABLE_MESSAGE_BUFFER_POOL
#endif
#endif

/*
 * \see https://wiki.alcf.anl.gov/parts/index.php/Blue_Gene/Q
 * \see https://svn.mcs.anl.gov/repos/ZeptoOS/trunk/BGP/comm/arch-runtime/arch/include/cnk/vmm.h
 * \see https://github.com/jedbrown/bgq-driver/blob/master/cnk/include/Config.h
 */

/*
 * The block size for the memory pool.
 */
#define CORE_MEMORY_POOL_MESSAGE_BUFFER_BLOCK_SIZE (2 * 1024 * 1024)

struct core_memory_pool_state {
    int test_profile_allocate_calls;
    int test_profile_free_calls;
};

/*
 * A memory pool for genomics.
 *
 * \see http://en.wikipedia.org/wiki/Memory_pool
 */
struct core_memory_pool {
    struct core_map recycle_bin;
    struct core_map allocated_blocks;
    struct core_set large_blocks;
    struct core_memory_block *current_block;
    struct core_queue ready_blocks;
    struct core_queue dried_blocks;

    uint32_t flags;
    size_t block_size;

    int name;

    uint64_t profile_allocated_byte_count;
    uint64_t profile_freed_byte_count;

    int profile_allocate_calls;
    int profile_free_calls;
};

void core_memory_pool_init(struct core_memory_pool *self, int block_size, int name);
void core_memory_pool_destroy(struct core_memory_pool *self);

void *core_memory_pool_allocate(struct core_memory_pool *self, size_t size);
int core_memory_pool_free(struct core_memory_pool *self, void *pointer);

void core_memory_pool_enable_tracking(struct core_memory_pool *self);
void core_memory_pool_disable_tracking(struct core_memory_pool *self);

void core_memory_pool_free_all(struct core_memory_pool *self);
void core_memory_pool_disable(struct core_memory_pool *self);

void core_memory_pool_disable_normalization(struct core_memory_pool *self);
void core_memory_pool_enable_normalization(struct core_memory_pool *self);
void core_memory_pool_enable_ephemeral_mode(struct core_memory_pool *self);

void core_memory_pool_disable_alignment(struct core_memory_pool *self);
void core_memory_pool_enable_alignment(struct core_memory_pool *self);
void core_memory_pool_print(struct core_memory_pool *self);

void core_memory_pool_examine(struct core_memory_pool *self);
void core_memory_pool_profile(struct core_memory_pool *self, int operation, size_t byte_count);

int core_memory_pool_has_leaks(struct core_memory_pool *self);
void core_memory_pool_begin(struct core_memory_pool *self, struct core_memory_pool_state *state);
void core_memory_pool_end(struct core_memory_pool *self, struct core_memory_pool_state *state,
                const char *name, const char *function, const char *file, int line);
int core_memory_pool_has_double_free(struct core_memory_pool *self);
int core_memory_pool_profile_allocate_count(struct core_memory_pool *self);
int core_memory_pool_profile_free_count(struct core_memory_pool *self);
void core_memory_pool_check_double_free(struct core_memory_pool *self,
                const char *function, const char *file, int line);
int core_memory_pool_profile_balance_count(struct core_memory_pool *self);

#endif
