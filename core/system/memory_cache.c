
#include "memory_cache.h"

#include <core/system/debugger.h>

#include <core/system/memory.h>

#include <stdio.h>

void core_memory_cache_add_block(struct core_memory_cache *self);

void core_memory_cache_init(struct core_memory_cache *self, int name, size_t size,
                size_t chunk_size)
{
    size = core_free_list_check_size(&self->free_list, size);

    CORE_DEBUGGER_ASSERT(size >= sizeof(struct core_free_list_element));
    self->name = name;

    core_free_list_init(&self->free_list);
    self->block = NULL;
    self->profile_allocate_call_count = 0;
    self->profile_free_call_count = 0;

    self->size = size;
    self->chunk_size = chunk_size;
}

void core_memory_cache_destroy(struct core_memory_cache *self)
{
    struct core_memory_block *block;
    struct core_memory_block *next;

    block = self->block;

    while (block != NULL) {
        next = core_memory_block_next(block);
        core_memory_block_destroy(block);
        core_memory_free(block, self->name);
        block = next;
    }

    self->block = NULL;
    core_free_list_destroy(&self->free_list);

    self->profile_allocate_call_count = 0;
    self->profile_free_call_count = 0;

    self->size = 0;
}

void *core_memory_cache_allocate(struct core_memory_cache *self, size_t size)
{
    void *pointer;

    size = core_free_list_check_size(&self->free_list, size);

    CORE_DEBUGGER_ASSERT(self->size > 0);
    CORE_DEBUGGER_ASSERT(size > 0);
    CORE_DEBUGGER_ASSERT(self->size == size);

    if (size != self->size)
        return NULL;

    ++self->profile_allocate_call_count;

    if (!core_free_list_empty(&self->free_list)) {
        pointer = core_free_list_remove(&self->free_list);

        CORE_DEBUGGER_ASSERT_NOT_NULL(pointer);

        return pointer;
    }

    /*
     * This is the first block.
     */
    if (self->block == NULL)
        core_memory_cache_add_block(self);

    CORE_DEBUGGER_ASSERT(self->block != NULL);

    pointer = core_memory_block_allocate(self->block, self->size);

    /*
     * The current head block is dried and can not allocate this request.
     */
    if (pointer == NULL) {
        core_memory_cache_add_block(self);
        pointer = core_memory_block_allocate(self->block, self->size);
    }

    CORE_DEBUGGER_ASSERT(pointer != NULL);

    return pointer;
}

void core_memory_cache_free(struct core_memory_cache *self, void *pointer)
{
    ++self->profile_free_call_count;

    core_free_list_add(&self->free_list, pointer);
}

void core_memory_cache_add_block(struct core_memory_cache *self)
{
    struct core_memory_block *block;

    block = core_memory_allocate(sizeof(struct core_memory_block), self->name);

    CORE_DEBUGGER_ASSERT(self->chunk_size > 0);

    core_memory_block_init(block, self->chunk_size, self->name);

#ifdef DEBUG_DISPLAY_BLOCK_INIT
    printf("DEBUG add block.\n");
#endif

    core_memory_block_set_next(block, self->block);
    self->block = block;
}

int core_memory_cache_balance(struct core_memory_cache *self)
{
    int balance;

    balance = 0;
    balance += self->profile_allocate_call_count;
    balance -= self->profile_free_call_count;

    return balance;
}

