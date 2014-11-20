
#ifndef CORE_MEMORY_CACHE_
#define CORE_MEMORY_CACHE_

#include <core/system/memory_block.h>

#include <core/structures/free_list.h>

#include <string.h> /* for size_t */

/*
 * This is a cache allocator based on some ideas from
 * the Linux Kernel Memory Cache (SLAB allocator).
 *
 * A single cache deals with objects of the same size, always.
 *
 * http://www.lehman.cuny.edu/cgi-bin/man-cgi?kmem_cache_alloc+9
 */
struct core_memory_cache {
    size_t chunk_size;
    size_t size;
    int name;
    struct core_free_list free_list;
    struct core_memory_block *block;
    int profile_allocate_call_count;
    int profile_free_call_count;
};

void core_memory_cache_init(struct core_memory_cache *self, int name, size_t size,
                size_t chunk_size);
void core_memory_cache_destroy(struct core_memory_cache *self);

void *core_memory_cache_allocate(struct core_memory_cache *self, size_t size);
void core_memory_cache_free(struct core_memory_cache *self, void *pointer);
int core_memory_cache_balance(struct core_memory_cache *self);

#endif
