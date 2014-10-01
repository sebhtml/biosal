
#include "memory_pool.h"

#include <core/system/tracer.h>
#include <core/system/debugger.h>

#include <core/helpers/bitmap.h>

#include <core/structures/queue.h>
#include <core/structures/map_iterator.h>

#include <stdio.h>
#include <inttypes.h>
#include <stdint.h>

/*
 * Some flags.
 */
#define FLAG_ENABLE_TRACKING 0
#define FLAG_DISABLED 1
#define FLAG_ENABLE_SEGMENT_NORMALIZATION 2
#define FLAG_ALIGN 3
#define FLAG_EPHEMERAL 4

#define OPERATION_ALLOCATE  0
#define OPERATION_FREE      1

#define MEMORY_MEMORY_POOL 0xc170626e

void biosal_memory_pool_init(struct biosal_memory_pool *self, int block_size, int name)
{
    biosal_map_init(&self->recycle_bin, sizeof(size_t), sizeof(struct biosal_queue));
    biosal_map_init(&self->allocated_blocks, sizeof(void *), sizeof(size_t));
    biosal_set_init(&self->large_blocks, sizeof(void *));

    self->current_block = NULL;
    self->name = name;

    biosal_queue_init(&self->dried_blocks, sizeof(struct biosal_memory_block *));
    biosal_queue_init(&self->ready_blocks, sizeof(struct biosal_memory_block *));

    self->block_size = block_size;

    /*
     * Configure flags
     */
    self->flags = 0;
    biosal_bitmap_set_bit_uint32_t(&self->flags, FLAG_ENABLE_TRACKING);
    biosal_bitmap_clear_bit_uint32_t(&self->flags, FLAG_DISABLED);
    biosal_bitmap_clear_bit_uint32_t(&self->flags, FLAG_ENABLE_SEGMENT_NORMALIZATION);
    biosal_bitmap_clear_bit_uint32_t(&self->flags, FLAG_ALIGN);
    biosal_bitmap_clear_bit_uint32_t(&self->flags, FLAG_EPHEMERAL);

    self->profile_allocated_byte_count = 0;
    self->profile_freed_byte_count = 0;
    self->profile_allocate_calls = 0;
    self->profile_free_calls = 0;
}

void biosal_memory_pool_destroy(struct biosal_memory_pool *self)
{
    struct biosal_queue *queue;
    struct biosal_map_iterator iterator;
    struct biosal_memory_block *block;

#ifdef BIOSAL_MEMORY_POOL_FIND_LEAKS
    BIOSAL_DEBUGGER_ASSERT(!biosal_memory_pool_has_leaks(self));
#endif

    /* destroy recycled objects
     */
    biosal_map_iterator_init(&iterator, &self->recycle_bin);

    while (biosal_map_iterator_has_next(&iterator)) {
        biosal_map_iterator_next(&iterator, NULL, (void **)&queue);

        biosal_queue_destroy(queue);
    }
    biosal_map_iterator_destroy(&iterator);
    biosal_map_destroy(&self->recycle_bin);

    /* destroy allocated blocks */
    biosal_map_destroy(&self->allocated_blocks);

    /* destroy dried blocks
     */
    while (biosal_queue_dequeue(&self->dried_blocks, &block)) {
        biosal_memory_block_destroy(block);
        biosal_memory_free(block, self->name);
    }
    biosal_queue_destroy(&self->dried_blocks);

    /* destroy ready blocks
     */
    while (biosal_queue_dequeue(&self->ready_blocks, &block)) {
        biosal_memory_block_destroy(block);
        biosal_memory_free(block, self->name);
    }
    biosal_queue_destroy(&self->ready_blocks);

    /* destroy the current block
     */
    if (self->current_block != NULL) {
        biosal_memory_block_destroy(self->current_block);
        biosal_memory_free(self->current_block, self->name);
        self->current_block = NULL;
    }

    biosal_set_destroy(&self->large_blocks);
}

void *biosal_memory_pool_allocate(struct biosal_memory_pool *self, size_t size)
{
    void *pointer;
    size_t new_size;
    int normalize;

    BIOSAL_DEBUGGER_ASSERT(size > 0);

    if (self == NULL) {
        return biosal_memory_allocate(size, MEMORY_MEMORY_POOL);
    }

#ifdef BIOSAL_DEBUGGER_ENABLE_ASSERT
    if (size < BIOSAL_MEMORY_MINIMUM) {
        printf("Error: too low %zu\n", size);
    }
    if (size > BIOSAL_MEMORY_MAXIMUM) {
        printf("Error: too high %zu\n", size);
    }
#endif
    BIOSAL_DEBUGGER_ASSERT(size >= BIOSAL_MEMORY_MINIMUM);
    BIOSAL_DEBUGGER_ASSERT(size <= BIOSAL_MEMORY_MAXIMUM);

    normalize = 0;

    /*
     * Normalize the length of the segment to be a power of 2
     * if the flag FLAG_ENABLE_SEGMENT_NORMALIZATION is set.
     */
    if (biosal_bitmap_get_bit_uint32_t(&self->flags, FLAG_ENABLE_SEGMENT_NORMALIZATION)) {
        normalize = 1;
    }

    /*
     * Normalize the segment if the flag FLAG_EPHEMERAL is set
     * and if the segment size is larger than block size.
     *
     * This is required because any size exceeding the capacity will go to the
     * operating system malloc/free directly so sizes should be
     * normalized.
     */

    if (size > self->block_size
              && biosal_bitmap_get_bit_uint32_t(&self->flags, FLAG_EPHEMERAL)) {
        normalize = 1;
    }

    if (normalize) {

        /*
         * The Blue Gene/Q seems to prefer powers of 2
         * otherwise fragmentation makes the system run out of memory.
         */
        new_size = biosal_memory_normalize_segment_length_power_of_2(size);
            /*
        new_size = biosal_memory_normalize_segment_length_page_size(size);
        */
#if 0
        printf("NORMALIZE %zu -> %zu\n", size, new_size);
#endif
        size = new_size;
    }

    BIOSAL_DEBUGGER_ASSERT(size >= BIOSAL_MEMORY_MINIMUM);
    BIOSAL_DEBUGGER_ASSERT(size <= BIOSAL_MEMORY_MAXIMUM);

    /*
     * Normalize the length so that it won't break alignment
     */
    /*
     * Finally, normalize so that the alignment is maintained.
     *
     * On Cray XE6 (AMD Opteron), or on Intel Xeon, this does not change
     * the correctness while it can lead to better performance.
     *
     * On IBM Blue Gene/Q (IBM PowerPC A2 1.6 GHz), unaligned communication
     * buffers produce incorrect behaviours.
     *
     * On Cetus and Mira, I noticed that MPI_Isend / MPI_Irecv and friends have very strange behavior
     * when the buffer are not aligned at all.
     *
     * In http://www.redbooks.ibm.com/redbooks/pdfs/sg247948.pdf
     *
     * section 6.2.7 Buffer alignment sensitivity says that Blue Gene/Q likes MPI buffers aligned on 32 bytes.
     *
     * So 32-byte and 64-byte alignment can give better performance.
     * But what is the necessary alignment for getting correct behavior ?
     */
    if (biosal_bitmap_get_bit_uint32_t(&self->flags, FLAG_ALIGN)) {

        new_size = biosal_memory_align(size);

        size = new_size;
    }

    BIOSAL_DEBUGGER_ASSERT(size >= BIOSAL_MEMORY_MINIMUM);
    BIOSAL_DEBUGGER_ASSERT(size <= BIOSAL_MEMORY_MAXIMUM);

    pointer = biosal_memory_pool_allocate_private(self, size);

    if (biosal_bitmap_get_bit_uint32_t(&self->flags, FLAG_ENABLE_TRACKING)) {
            biosal_map_add_value(&self->allocated_blocks, &pointer, &size);
    }

    biosal_memory_pool_profile(self, OPERATION_ALLOCATE, size);

    if (pointer == NULL) {
        printf("Error, requested %zu bytes, returned pointer is NULL\n",
                        size);

        biosal_tracer_print_stack_backtrace();

        exit(1);
    }

    return pointer;
}

void *biosal_memory_pool_allocate_private(struct biosal_memory_pool *self, size_t size)
{
    struct biosal_queue *queue;
    void *pointer;

    if (size == 0) {
        return NULL;
    }

    if (biosal_bitmap_get_bit_uint32_t(&self->flags, FLAG_DISABLED)) {
        return biosal_memory_allocate(size, self->name);
    }

    /*
     * First, check if the size is larger than the maximum size.
     * If memory blocks can not fulfil the need, use the memory system
     * directly.
     */

    if (size >= self->block_size) {
        pointer = biosal_memory_allocate(size, self->name);

        biosal_set_add(&self->large_blocks, &pointer);

        return pointer;
    }

    queue = NULL;

    if (biosal_bitmap_get_bit_uint32_t(&self->flags, FLAG_ENABLE_TRACKING)) {
        queue = biosal_map_get(&self->recycle_bin, &size);
    }

    /* recycling is good for the environment
     */
    if (queue != NULL && biosal_queue_dequeue(queue, &pointer)) {

#ifdef BIOSAL_MEMORY_POOL_DISCARD_EMPTY_QUEUES
        if (biosal_queue_empty(queue)) {
            biosal_queue_destroy(queue);
            biosal_map_delete(&self->recycle_bin, &size);
        }
#endif

        return pointer;
    }

    if (self->current_block == NULL) {

        biosal_memory_pool_add_block(self);
    }

    pointer = biosal_memory_block_allocate(self->current_block, size);

    /* the current block is exausted...
     */
    if (pointer == NULL) {
        biosal_queue_enqueue(&self->dried_blocks, &self->current_block);
        self->current_block = NULL;

        biosal_memory_pool_add_block(self);

        pointer = biosal_memory_block_allocate(self->current_block, size);
    }

    return pointer;
}

void biosal_memory_pool_add_block(struct biosal_memory_pool *self)
{
    /* Try to pick a block in the ready block list.
     * Otherwise, create one on-demand today.
     */
    if (!biosal_queue_dequeue(&self->ready_blocks, &self->current_block)) {
        self->current_block = biosal_memory_allocate(sizeof(struct biosal_memory_block), self->name);
        biosal_memory_block_init(self->current_block, self->block_size);
    }
}

void biosal_memory_pool_free(struct biosal_memory_pool *self, void *pointer)
{
    size_t size;

    BIOSAL_DEBUGGER_ASSERT(pointer != NULL);

    if (self == NULL) {
        biosal_memory_free(pointer, MEMORY_MEMORY_POOL);
        return;
    }

    size = 0;

    biosal_memory_pool_free_private(self, pointer);

    if (biosal_map_get_value(&self->allocated_blocks, &pointer, &size)) {

        biosal_map_delete(&self->allocated_blocks, &pointer);
    }

    /*
     * find out the actual size.
     */
    biosal_memory_pool_profile(self, OPERATION_FREE, size);
}

void biosal_memory_pool_free_private(struct biosal_memory_pool *self, void *pointer)
{
    struct biosal_queue *queue;
    size_t size;

    if (biosal_bitmap_get_bit_uint32_t(&self->flags, FLAG_DISABLED)) {
        biosal_memory_free(pointer, self->name);
        return;
    }

    /* Verify if the pointer is a large block not managed by one of the memory
     * blocks
     */
    if (biosal_set_find(&self->large_blocks, &pointer)) {

        biosal_memory_free(pointer, self->name);
        biosal_set_delete(&self->large_blocks, &pointer);
        return;
    }

    /*
     * Return immediately if memory allocation tracking is disabled.
     * For example, the ephemeral memory component of a worker
     * disable tracking (flag FLAG_ENABLE_TRACKING = 0). To free memory,
     * for the ephemeral memory, biosal_memory_pool_free_all is
     * used.
     */
    if (!biosal_bitmap_get_bit_uint32_t(&self->flags, FLAG_ENABLE_TRACKING)) {
        return;
    }

    /*
     * This was not allocated by this pool.
     */
    if (!biosal_map_get_value(&self->allocated_blocks, &pointer, &size)) {
        return;
    }

    queue = biosal_map_get(&self->recycle_bin, &size);

    if (queue == NULL) {
        queue = biosal_map_add(&self->recycle_bin, &size);
        biosal_queue_init(queue, sizeof(void *));
    }

    biosal_queue_enqueue(queue, &pointer);
}

void biosal_memory_pool_disable_tracking(struct biosal_memory_pool *self)
{
    biosal_bitmap_clear_bit_uint32_t(&self->flags, FLAG_ENABLE_TRACKING);
}

void biosal_memory_pool_enable_normalization(struct biosal_memory_pool *self)
{
    biosal_bitmap_set_bit_uint32_t(&self->flags, FLAG_ENABLE_SEGMENT_NORMALIZATION);
}

void biosal_memory_pool_disable_normalization(struct biosal_memory_pool *self)
{
    biosal_bitmap_clear_bit_uint32_t(&self->flags, FLAG_ENABLE_SEGMENT_NORMALIZATION);
}

void biosal_memory_pool_enable_alignment(struct biosal_memory_pool *self)
{
    biosal_bitmap_set_bit_uint32_t(&self->flags, FLAG_ALIGN);
}

void biosal_memory_pool_disable_alignment(struct biosal_memory_pool *self)
{
    biosal_bitmap_clear_bit_uint32_t(&self->flags, FLAG_ALIGN);
}

void biosal_memory_pool_enable_tracking(struct biosal_memory_pool *self)
{
    biosal_bitmap_set_bit_uint32_t(&self->flags, FLAG_ENABLE_TRACKING);
}

void biosal_memory_pool_free_all(struct biosal_memory_pool *self)
{
    struct biosal_memory_block *block;
    int i;
    int size;

#ifdef BIOSAL_DEBUGGER_ENABLE_ASSERT
    if (biosal_memory_pool_has_leaks(self)) {
        biosal_memory_pool_examine(self);
    }

    BIOSAL_DEBUGGER_ASSERT(!biosal_memory_pool_has_leaks(self));
#endif

    /*
     * Reset the current block
     */
    if (self->current_block != NULL) {
        biosal_memory_block_free_all(self->current_block);
    }

    /*
     * Reset all ready blocks
     */
    size = biosal_queue_size(&self->ready_blocks);
    i = 0;
    while (i < size
                   && biosal_queue_dequeue(&self->ready_blocks, &block)) {
        biosal_memory_block_free_all(block);
        biosal_queue_enqueue(&self->ready_blocks, &block);

        i++;
    }

    /*
     * Reset all dried blocks
     */
    while (biosal_queue_dequeue(&self->dried_blocks, &block)) {
        biosal_memory_block_free_all(block);
        biosal_queue_enqueue(&self->ready_blocks, &block);
    }

    /*
     * Reset current structures.
     */
    if (biosal_bitmap_get_bit_uint32_t(&self->flags, FLAG_ENABLE_TRACKING)) {
        biosal_map_clear(&self->allocated_blocks);
        biosal_map_clear(&self->recycle_bin);
    }

    if (!biosal_bitmap_get_bit_uint32_t(&self->flags, FLAG_DISABLED)) {
        biosal_set_clear(&self->large_blocks);
    }
}

void biosal_memory_pool_disable(struct biosal_memory_pool *self)
{
    biosal_bitmap_set_bit_uint32_t(&self->flags, FLAG_DISABLED);
}

void biosal_memory_pool_print(struct biosal_memory_pool *self)
{
    int block_count;
    uint64_t byte_count;

    block_count = 0;

    if (self->current_block != NULL) {
        ++block_count;
    }

    block_count += biosal_queue_size(&self->dried_blocks);
    block_count += biosal_queue_size(&self->ready_blocks);

    byte_count = (uint64_t)block_count * (uint64_t)self->block_size;

    printf("PRINT POOL Name= 0x%x memory_pool BlockSize: %d BlockCount: %d ByteCount: %" PRIu64 "\n",
                    self->name,
                    (int)self->block_size,
                    block_count,
                    byte_count);
}

void biosal_memory_pool_enable_ephemeral_mode(struct biosal_memory_pool *self)
{
    biosal_bitmap_set_bit_uint32_t(&self->flags, FLAG_EPHEMERAL);
}

void biosal_memory_pool_set_name(struct biosal_memory_pool *self, int name)
{
    self->name = name;
}

void biosal_memory_pool_examine(struct biosal_memory_pool *self)
{
    printf("DEBUG_POOL Name= 0x%x"
                    " AllocatedPointerCount= %d (%d - %d)"
                    " AllocatedByteCount= %" PRIu64 " (%" PRIu64 " - %" PRIu64 ")"
                    "\n",

                    self->name,

                    self->profile_allocate_calls - self->profile_free_calls,
                    self->profile_allocate_calls, self->profile_free_calls,

                    self->profile_allocated_byte_count - self->profile_freed_byte_count,
                    self->profile_allocated_byte_count, self->profile_freed_byte_count);

#if 0
    biosal_memory_pool_print(self);
#endif
}

void biosal_memory_pool_profile(struct biosal_memory_pool *self, int operation, size_t byte_count)
{
    if (operation == OPERATION_ALLOCATE) {
        ++self->profile_allocate_calls;
        self->profile_allocated_byte_count += byte_count;
    } else if (operation == OPERATION_FREE) {
        ++self->profile_free_calls;
        self->profile_freed_byte_count += byte_count;
    }

#ifdef BIOSAL_DEBUGGER_CHECK_DOUBLE_FREE_IN_POOL
#ifdef BIOSAL_DEBUGGER_ENABLE_ASSERT
    if (!(self->profile_allocate_calls >= self->profile_free_calls)) {
        biosal_memory_pool_examine(self);
    }
#endif
    BIOSAL_DEBUGGER_ASSERT(self->profile_allocate_calls >= self->profile_free_calls);
#endif
}

int biosal_memory_pool_has_leaks(struct biosal_memory_pool *self)
{
#ifdef BIOSAL_DEBUGGER_CHECK_LEAKS_IN_POOL
    return self->profile_allocate_calls != self->profile_free_calls;
#else
    return 0;
#endif
}

void biosal_memory_pool_begin(struct biosal_memory_pool *self, struct biosal_memory_pool_state *state)
{
    state->test_profile_allocate_calls = self->profile_allocate_calls;
    state->test_profile_free_calls = self->profile_free_calls;
}

void biosal_memory_pool_end(struct biosal_memory_pool *self, struct biosal_memory_pool_state *state,
                const char *name, const char *function, const char *file, int line)
{
    int allocate_calls;
    int free_calls;

    allocate_calls = self->profile_allocate_calls - state->test_profile_allocate_calls;
    free_calls = self->profile_free_calls - state->test_profile_free_calls;

    if (allocate_calls != free_calls) {
        printf("Error, saved pool state \"%s\" (%s %s %d) reveals leaks: allocate_calls %d free_calls %d (balance: %d)\n",
                        name, function, file, line, allocate_calls, free_calls,
                        allocate_calls - free_calls);
    }

    BIOSAL_DEBUGGER_ASSERT(allocate_calls == free_calls);
}

int biosal_memory_pool_has_double_free(struct biosal_memory_pool *self)
{
    return self->profile_allocate_calls < self->profile_free_calls;
}

int biosal_memory_pool_profile_allocate_count(struct biosal_memory_pool *self)
{
    return self->profile_allocate_calls;
}

int biosal_memory_pool_profile_free_count(struct biosal_memory_pool *self)
{
    return self->profile_free_calls;
}

void biosal_memory_pool_check_double_free(struct biosal_memory_pool *self,
        const char *function, const char *file, int line)
{
    int balance;

    balance = 0;

    balance += self->profile_allocate_calls;
    balance -= self->profile_free_calls;

    if (self->profile_free_calls > self->profile_allocate_calls) {
        printf("%s %s %d INFO profile_allocate_calls %d profile_free_calls %d balance %d\n",
                        function, file, line,
                        self->profile_allocate_calls, self->profile_free_calls, balance);
    }

    BIOSAL_DEBUGGER_ASSERT(self->profile_allocate_calls >= self->profile_free_calls);
}

int biosal_memory_pool_profile_balance_count(struct biosal_memory_pool *self)
{
    return self->profile_allocate_calls - self->profile_free_calls;
}
