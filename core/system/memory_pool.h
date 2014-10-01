
#ifndef BIOSAL_MEMORY_POOL_H
#define BIOSAL_MEMORY_POOL_H

#include "memory.h"
#include "memory_block.h"

#include <core/structures/map.h>
#include <core/structures/set.h>
#include <core/structures/queue.h>

#include <stdint.h>

/*
*/

#define BIOSAL_MEMORY_POOL_NAME_WORKER_EPHEMERAL  0x2ee1c5a6
#define BIOSAL_MEMORY_POOL_NAME_WORKER_OUTBOUND   0x46d316e4
#define BIOSAL_MEMORY_POOL_NAME_NODE_INBOUND      0xee1344f0
#define BIOSAL_MEMORY_POOL_NAME_NODE_OUTBOUND     0xf3ad5880
#define BIOSAL_MEMORY_POOL_NAME_NONE              0xcef49361
#define BIOSAL_MEMORY_POOL_NAME_ACTORS            0x37ddf367
#define BIOSAL_MEMORY_POOL_NAME_SEQUENCE_STORE    0x84a83916
#define BIOSAL_MEMORY_POOL_NAME_OTHER             0x8b5b96d6
#define BIOSAL_MEMORY_POOL_NAME_GRAPH_STORE       0x89e9235d

/*
 * Disable particular memory pool on IBM
 * Blue Gene/Q because otherwise it does not
 * work.
 */
#if defined(__bgq__)
#define BIOSAL_MEMORY_POOL_DISABLE_MESSAGE_BUFFER_POOL
#endif

/*
 * \see https://wiki.alcf.anl.gov/parts/index.php/Blue_Gene/Q
 * \see https://svn.mcs.anl.gov/repos/ZeptoOS/trunk/BGP/comm/arch-runtime/arch/include/cnk/vmm.h
 * \see https://github.com/jedbrown/bgq-driver/blob/master/cnk/include/Config.h
 */

/*
 * The block size for the memory pool.
 */
#define BIOSAL_MEMORY_POOL_MESSAGE_BUFFER_BLOCK_SIZE (2 * 1024 * 1024)

struct biosal_memory_pool_state {
    int test_profile_allocate_calls;
    int test_profile_free_calls;
};

/*
 * A memory pool for genomics.
 *
 * \see http://en.wikipedia.org/wiki/Memory_pool
 */
struct biosal_memory_pool {
    struct biosal_map recycle_bin;
    struct biosal_map allocated_blocks;
    struct biosal_set large_blocks;
    struct biosal_memory_block *current_block;
    struct biosal_queue ready_blocks;
    struct biosal_queue dried_blocks;

    uint32_t flags;
    size_t block_size;

    int name;

    uint64_t profile_allocated_byte_count;
    uint64_t profile_freed_byte_count;

    int profile_allocate_calls;
    int profile_free_calls;
};

void biosal_memory_pool_init(struct biosal_memory_pool *self, int block_size, int name);
void biosal_memory_pool_destroy(struct biosal_memory_pool *self);

void *biosal_memory_pool_allocate(struct biosal_memory_pool *self, size_t size);
void *biosal_memory_pool_allocate_private(struct biosal_memory_pool *self, size_t size);

void biosal_memory_pool_free(struct biosal_memory_pool *self, void *pointer);
void biosal_memory_pool_free_private(struct biosal_memory_pool *self, void *pointer);


void biosal_memory_pool_enable_tracking(struct biosal_memory_pool *self);
void biosal_memory_pool_disable_tracking(struct biosal_memory_pool *self);

void biosal_memory_pool_free_all(struct biosal_memory_pool *self);
void biosal_memory_pool_disable(struct biosal_memory_pool *self);

void biosal_memory_pool_add_block(struct biosal_memory_pool *self);

void biosal_memory_pool_disable_normalization(struct biosal_memory_pool *self);
void biosal_memory_pool_enable_normalization(struct biosal_memory_pool *self);
void biosal_memory_pool_enable_ephemeral_mode(struct biosal_memory_pool *self);

void biosal_memory_pool_disable_alignment(struct biosal_memory_pool *self);
void biosal_memory_pool_enable_alignment(struct biosal_memory_pool *self);
void biosal_memory_pool_print(struct biosal_memory_pool *self);
void biosal_memory_pool_set_name(struct biosal_memory_pool *self, int name);

void biosal_memory_pool_examine(struct biosal_memory_pool *self);
void biosal_memory_pool_profile(struct biosal_memory_pool *self, int operation, size_t byte_count);

int biosal_memory_pool_has_leaks(struct biosal_memory_pool *self);
void biosal_memory_pool_begin(struct biosal_memory_pool *self, struct biosal_memory_pool_state *state);
void biosal_memory_pool_end(struct biosal_memory_pool *self, struct biosal_memory_pool_state *state,
                const char *name, const char *function, const char *file, int line);
int biosal_memory_pool_has_double_free(struct biosal_memory_pool *self);
int biosal_memory_pool_profile_allocate_count(struct biosal_memory_pool *self);
int biosal_memory_pool_profile_free_count(struct biosal_memory_pool *self);
void biosal_memory_pool_check_double_free(struct biosal_memory_pool *self,
                const char *function, const char *file, int line);
int biosal_memory_pool_profile_balance_count(struct biosal_memory_pool *self);

#endif
