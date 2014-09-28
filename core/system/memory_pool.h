
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

#define BSAL_MEMORY_POOL_NAME_WORKER_EPHEMERAL  0x2ee1c5a6
#define BSAL_MEMORY_POOL_NAME_WORKER_OUTBOUND   0x46d316e4
#define BSAL_MEMORY_POOL_NAME_NODE_INBOUND      0xee1344f0
#define BSAL_MEMORY_POOL_NAME_NODE_OUTBOUND     0xf3ad5880
#define BSAL_MEMORY_POOL_NAME_NONE              0xcef49361
#define BSAL_MEMORY_POOL_NAME_ACTORS            0x37ddf367
#define BSAL_MEMORY_POOL_NAME_SEQUENCE_STORE    6
#define BSAL_MEMORY_POOL_NAME_OTHER             7
#define BSAL_MEMORY_POOL_NAME_GRAPH_STORE       8

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

/*
 * The block size for the memory pool.
 */
#define BSAL_MEMORY_POOL_MESSAGE_BUFFER_BLOCK_SIZE (2 * 1024 * 1024)

struct bsal_memory_pool_state {
    int test_profile_allocate_calls;
    int test_profile_free_calls;
};

/*
 * A memory pool for genomics.
 *
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
    size_t block_size;

    int name;

    uint64_t profile_allocated_byte_count;
    uint64_t profile_freed_byte_count;

    int profile_allocate_calls;
    int profile_free_calls;
};

void bsal_memory_pool_init(struct bsal_memory_pool *self, int block_size, int name);
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
void bsal_memory_pool_enable_ephemeral_mode(struct bsal_memory_pool *self);

void bsal_memory_pool_disable_alignment(struct bsal_memory_pool *self);
void bsal_memory_pool_enable_alignment(struct bsal_memory_pool *self);
void bsal_memory_pool_print(struct bsal_memory_pool *self);
void bsal_memory_pool_set_name(struct bsal_memory_pool *self, int name);

void bsal_memory_pool_examine(struct bsal_memory_pool *self);
void bsal_memory_pool_profile(struct bsal_memory_pool *self, int operation, size_t byte_count);

int bsal_memory_pool_has_leaks(struct bsal_memory_pool *self);
void bsal_memory_pool_begin(struct bsal_memory_pool *self, struct bsal_memory_pool_state *state);
void bsal_memory_pool_end(struct bsal_memory_pool *self, struct bsal_memory_pool_state *state,
                const char *name, const char *function, const char *file, int line);
int bsal_memory_pool_has_double_free(struct bsal_memory_pool *self);
int bsal_memory_pool_profile_allocate_count(struct bsal_memory_pool *self);
int bsal_memory_pool_profile_free_count(struct bsal_memory_pool *self);
void bsal_memory_pool_check_double_free(struct bsal_memory_pool *self,
                const char *function, const char *file, int line);
int bsal_memory_pool_profile_balance_count(struct bsal_memory_pool *self);


#endif
