
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

/*
 * Parameters for debugging.
 */
/*
#define CHECK_DOUBLE_FREE
#define CHECK_LEAKS
*/
void bsal_memory_pool_init(struct bsal_memory_pool *self, int block_size, int name)
{
    bsal_map_init(&self->recycle_bin, sizeof(int), sizeof(struct bsal_queue));
    bsal_map_init(&self->allocated_blocks, sizeof(void *), sizeof(int));
    bsal_set_init(&self->large_blocks, sizeof(void *));

    self->current_block = NULL;
    self->profile_name = name;

    bsal_queue_init(&self->dried_blocks, sizeof(struct bsal_memory_block *));
    bsal_queue_init(&self->ready_blocks, sizeof(struct bsal_memory_block *));

    self->block_size = block_size;

    /*
     * Configure flags
     */
    self->flags = 0;
    bsal_bitmap_set_bit_uint32_t(&self->flags, FLAG_ENABLE_TRACKING);
    bsal_bitmap_clear_bit_uint32_t(&self->flags, FLAG_DISABLED);
    bsal_bitmap_clear_bit_uint32_t(&self->flags, FLAG_ENABLE_SEGMENT_NORMALIZATION);
    bsal_bitmap_clear_bit_uint32_t(&self->flags, FLAG_ALIGN);
    bsal_bitmap_clear_bit_uint32_t(&self->flags, FLAG_EPHEMERAL);

    self->profile_allocated_byte_count = 0;
    self->profile_freed_byte_count = 0;
    self->profile_allocate_calls = 0;
    self->profile_free_calls = 0;

    self->snapshot_profile_allocate_calls = self->profile_allocate_calls;
    self->snapshot_profile_free_calls = self->profile_free_calls;
}

void bsal_memory_pool_destroy(struct bsal_memory_pool *self)
{
    struct bsal_queue *queue;
    struct bsal_map_iterator iterator;
    struct bsal_memory_block *block;

#ifdef BSAL_MEMORY_POOL_FIND_LEAKS
    BSAL_DEBUGGER_ASSERT(!bsal_memory_pool_has_leaks(self));
#endif

    /* destroy recycled objects
     */
    bsal_map_iterator_init(&iterator, &self->recycle_bin);

    while (bsal_map_iterator_has_next(&iterator)) {
        bsal_map_iterator_next(&iterator, NULL, (void **)&queue);

        bsal_queue_destroy(queue);
    }
    bsal_map_iterator_destroy(&iterator);
    bsal_map_destroy(&self->recycle_bin);

    /* destroy allocated blocks */
    bsal_map_destroy(&self->allocated_blocks);

    /* destroy dried blocks
     */
    while (bsal_queue_dequeue(&self->dried_blocks, &block)) {
        bsal_memory_block_destroy(block);
        bsal_memory_free(block);
    }
    bsal_queue_destroy(&self->dried_blocks);

    /* destroy ready blocks
     */
    while (bsal_queue_dequeue(&self->ready_blocks, &block)) {
        bsal_memory_block_destroy(block);
        bsal_memory_free(block);
    }
    bsal_queue_destroy(&self->ready_blocks);

    /* destroy the current block
     */
    if (self->current_block != NULL) {
        bsal_memory_block_destroy(self->current_block);
        bsal_memory_free(self->current_block);
        self->current_block = NULL;
    }

    bsal_set_destroy(&self->large_blocks);
}

void *bsal_memory_pool_allocate(struct bsal_memory_pool *self, size_t size)
{
    void *pointer;
    size_t new_size;
    int normalize;

#ifdef BSAL_DEBUGGER_ENABLE_ASSERT
    if (size < BSAL_MEMORY_MINIMUM) {
        printf("Error: too low %zu\n", size);
    }
    if (size > BSAL_MEMORY_MAXIMUM) {
        printf("Error: too high %zu\n", size);
    }
#endif
    BSAL_DEBUGGER_ASSERT(size >= BSAL_MEMORY_MINIMUM);
    BSAL_DEBUGGER_ASSERT(size <= BSAL_MEMORY_MAXIMUM);

    normalize = 0;

    /*
     * Normalize the length of the segment to be a power of 2
     * if the flag FLAG_ENABLE_SEGMENT_NORMALIZATION is set.
     */
    if (self != NULL
                 && bsal_bitmap_get_bit_uint32_t(&self->flags, FLAG_ENABLE_SEGMENT_NORMALIZATION)) {
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

    if (self != NULL
              && size > self->block_size
              && bsal_bitmap_get_bit_uint32_t(&self->flags, FLAG_EPHEMERAL)) {
        normalize = 1;
    }

    if (normalize) {

        /*
         * The Blue Gene/Q seems to prefer powers of 2
         * otherwise fragmentation makes the system run out of memory.
         */
        new_size = bsal_memory_normalize_segment_length_power_of_2(size);
            /*
        new_size = bsal_memory_normalize_segment_length_page_size(size);
        */
#if 0
        printf("NORMALIZE %zu -> %zu\n", size, new_size);
#endif
        size = new_size;
    }

    BSAL_DEBUGGER_ASSERT(size >= BSAL_MEMORY_MINIMUM);
    BSAL_DEBUGGER_ASSERT(size <= BSAL_MEMORY_MAXIMUM);

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
    if (self != NULL
                && bsal_bitmap_get_bit_uint32_t(&self->flags, FLAG_ALIGN)) {

        new_size = bsal_memory_align(size);

        size = new_size;
    }

    BSAL_DEBUGGER_ASSERT(size >= BSAL_MEMORY_MINIMUM);
    BSAL_DEBUGGER_ASSERT(size <= BSAL_MEMORY_MAXIMUM);

    pointer = bsal_memory_pool_allocate_private(self, size);

    if (pointer == NULL) {
        printf("Error, requested %zu bytes, returned pointer is NULL\n",
                        size);

        bsal_tracer_print_stack_backtrace();

        exit(1);
    }

    return pointer;
}

void *bsal_memory_pool_allocate_private(struct bsal_memory_pool *self, size_t size)
{
    struct bsal_queue *queue;
    void *pointer;

    if (size == 0) {
        return NULL;
    }

    if (self == NULL) {
        return bsal_memory_allocate(size);
    }

    bsal_memory_pool_profile(self, OPERATION_ALLOCATE, size);

    if (bsal_bitmap_get_bit_uint32_t(&self->flags, FLAG_DISABLED)) {
        return bsal_memory_allocate(size);
    }

    /*
     * First, check if the size is larger than the maximum size.
     * If memory blocks can not fulfil the need, use the memory system
     * directly.
     */

    if (size >= (size_t)self->block_size) {
        pointer = bsal_memory_allocate(size);

        bsal_set_add(&self->large_blocks, &pointer);

        return pointer;
    }

    queue = NULL;

    if (bsal_bitmap_get_bit_uint32_t(&self->flags, FLAG_ENABLE_TRACKING)) {
        queue = bsal_map_get(&self->recycle_bin, &size);
    }

    /* recycling is good for the environment
     */
    if (queue != NULL && bsal_queue_dequeue(queue, &pointer)) {

        if (bsal_bitmap_get_bit_uint32_t(&self->flags, FLAG_ENABLE_TRACKING)) {
            bsal_map_add_value(&self->allocated_blocks, &pointer, &size);
        }

#ifdef BSAL_MEMORY_POOL_DISCARD_EMPTY_QUEUES
        if (bsal_queue_empty(queue)) {
            bsal_queue_destroy(queue);
            bsal_map_delete(&self->recycle_bin, &size);
        }
#endif

        return pointer;
    }

    if (self->current_block == NULL) {

        bsal_memory_pool_add_block(self);
    }

    pointer = bsal_memory_block_allocate(self->current_block, size);

    /* the current block is exausted...
     */
    if (pointer == NULL) {
        bsal_queue_enqueue(&self->dried_blocks, &self->current_block);
        self->current_block = NULL;

        bsal_memory_pool_add_block(self);

        pointer = bsal_memory_block_allocate(self->current_block, size);
    }

    if (bsal_bitmap_get_bit_uint32_t(&self->flags, FLAG_ENABLE_TRACKING)) {
        bsal_map_add_value(&self->allocated_blocks, &pointer, &size);
    }

    return pointer;
}

void bsal_memory_pool_add_block(struct bsal_memory_pool *self)
{
    /* Try to pick a block in the ready block list.
     * Otherwise, create one on-demand today.
     */
    if (!bsal_queue_dequeue(&self->ready_blocks, &self->current_block)) {
        self->current_block = bsal_memory_allocate(sizeof(struct bsal_memory_block));
        bsal_memory_block_init(self->current_block, self->block_size);
    }
}

void bsal_memory_pool_free(struct bsal_memory_pool *self, void *pointer)
{
    struct bsal_queue *queue;
    int size;

    if (pointer == NULL) {
        return;
    }

    if (self == NULL) {
        bsal_memory_free(pointer);
        return;
    }

    /*
     * TODO: find out the actual size.
     */
    bsal_memory_pool_profile(self, OPERATION_FREE, 0);

    if (bsal_bitmap_get_bit_uint32_t(&self->flags, FLAG_DISABLED)) {
        bsal_memory_free(pointer);
        return;
    }

    /* Verify if the pointer is a large block not managed by one of the memory
     * blocks
     */
    if (bsal_set_find(&self->large_blocks, &pointer)) {

        bsal_memory_free(pointer);
        bsal_set_delete(&self->large_blocks, &pointer);
        return;
    }

    /*
     * Return immediately if memory allocation tracking is disabled.
     * For example, the ephemeral memory component of a worker
     * disable tracking (flag FLAG_ENABLE_TRACKING = 0). To free memory,
     * for the ephemeral memory, bsal_memory_pool_free_all is
     * used.
     */
    if (!bsal_bitmap_get_bit_uint32_t(&self->flags, FLAG_ENABLE_TRACKING)) {
        return;
    }

    /*
     * This was not allocated by this pool.
     */
    if (!bsal_map_get_value(&self->allocated_blocks, &pointer, &size)) {
        return;
    }

    queue = bsal_map_get(&self->recycle_bin, &size);

    if (queue == NULL) {
        queue = bsal_map_add(&self->recycle_bin, &size);
        bsal_queue_init(queue, sizeof(void *));
    }

    bsal_queue_enqueue(queue, &pointer);

    bsal_map_delete(&self->allocated_blocks, &pointer);
}

void bsal_memory_pool_disable_tracking(struct bsal_memory_pool *self)
{
    bsal_bitmap_clear_bit_uint32_t(&self->flags, FLAG_ENABLE_TRACKING);
}

void bsal_memory_pool_enable_normalization(struct bsal_memory_pool *self)
{
    bsal_bitmap_set_bit_uint32_t(&self->flags, FLAG_ENABLE_SEGMENT_NORMALIZATION);
}

void bsal_memory_pool_disable_normalization(struct bsal_memory_pool *self)
{
    bsal_bitmap_clear_bit_uint32_t(&self->flags, FLAG_ENABLE_SEGMENT_NORMALIZATION);
}

void bsal_memory_pool_enable_alignment(struct bsal_memory_pool *self)
{
    bsal_bitmap_set_bit_uint32_t(&self->flags, FLAG_ALIGN);
}

void bsal_memory_pool_disable_alignment(struct bsal_memory_pool *self)
{
    bsal_bitmap_clear_bit_uint32_t(&self->flags, FLAG_ALIGN);
}

void bsal_memory_pool_enable_tracking(struct bsal_memory_pool *self)
{
    bsal_bitmap_set_bit_uint32_t(&self->flags, FLAG_ENABLE_TRACKING);
}

void bsal_memory_pool_free_all(struct bsal_memory_pool *self)
{
    struct bsal_memory_block *block;
    int i;
    int size;

#ifdef BSAL_DEBUGGER_ENABLE_ASSERT
    if (bsal_memory_pool_has_leaks(self)) {
        bsal_memory_pool_examine(self);
    }

    BSAL_DEBUGGER_ASSERT(!bsal_memory_pool_has_leaks(self));
#endif

    self->snapshot_profile_allocate_calls = self->profile_allocate_calls;
    self->snapshot_profile_free_calls = self->profile_free_calls;

    /*
     * Reset the current block
     */
    if (self->current_block != NULL) {
        bsal_memory_block_free_all(self->current_block);
    }

    /*
     * Reset all ready blocks
     */
    size = bsal_queue_size(&self->ready_blocks);
    i = 0;
    while (i < size
                   && bsal_queue_dequeue(&self->ready_blocks, &block)) {
        bsal_memory_block_free_all(block);
        bsal_queue_enqueue(&self->ready_blocks, &block);

        i++;
    }

    /*
     * Reset all dried blocks
     */
    while (bsal_queue_dequeue(&self->dried_blocks, &block)) {
        bsal_memory_block_free_all(block);
        bsal_queue_enqueue(&self->ready_blocks, &block);
    }

    /*
     * Reset current structures.
     */
    if (bsal_bitmap_get_bit_uint32_t(&self->flags, FLAG_ENABLE_TRACKING)) {
        bsal_map_clear(&self->allocated_blocks);
        bsal_map_clear(&self->recycle_bin);
    }

    if (!bsal_bitmap_get_bit_uint32_t(&self->flags, FLAG_DISABLED)) {
        bsal_set_clear(&self->large_blocks);
    }
}

void bsal_memory_pool_disable(struct bsal_memory_pool *self)
{
    bsal_bitmap_set_bit_uint32_t(&self->flags, FLAG_DISABLED);
}

void bsal_memory_pool_print(struct bsal_memory_pool *self)
{
    int block_count;
    uint64_t byte_count;

    block_count = 0;

    if (self->current_block != NULL) {
        ++block_count;
    }

    block_count += bsal_queue_size(&self->dried_blocks);
    block_count += bsal_queue_size(&self->ready_blocks);

    byte_count = (uint64_t)block_count * (uint64_t)self->block_size;

    printf("EXAMINE memory_pool BlockSize: %d BlockCount: %d ByteCount: %" PRIu64 "\n",
                    (int)self->block_size,
                    block_count,
                    byte_count);
}

void bsal_memory_pool_enable_ephemeral_mode(struct bsal_memory_pool *self)
{
    bsal_bitmap_set_bit_uint32_t(&self->flags, FLAG_EPHEMERAL);
}

void bsal_memory_pool_set_name(struct bsal_memory_pool *self, int name)
{
    self->profile_name = name;
}

void bsal_memory_pool_examine(struct bsal_memory_pool *self)
{
    printf("DEBUG_POOL Name= %d"
                    " SinceSnapshotActiveSegments= %d (%d - %d)"
                    " ActiveSegments= %d (%d - %d)"
                    " SnapshotActiveSegments= %d (%d - %d)"
                    " AllocatedBytes= %" PRIu64 " (%" PRIu64 " - %" PRIu64 ")"
                    "\n",

                    self->profile_name,

                    (self->profile_allocate_calls - self->snapshot_profile_allocate_calls) -
                    (self->profile_free_calls - self->snapshot_profile_free_calls),
                    (self->profile_allocate_calls - self->snapshot_profile_allocate_calls),
                    (self->profile_free_calls - self->snapshot_profile_free_calls),

                    self->profile_allocate_calls - self->profile_free_calls,
                    self->profile_allocate_calls, self->profile_free_calls,

                    self->snapshot_profile_allocate_calls - self->snapshot_profile_free_calls,
                    self->snapshot_profile_allocate_calls, self->snapshot_profile_free_calls,

                    self->profile_allocated_byte_count - self->profile_freed_byte_count,
                    self->profile_allocated_byte_count, self->profile_freed_byte_count);

}

void bsal_memory_pool_profile(struct bsal_memory_pool *self, int operation, size_t byte_count)
{
    if (operation == OPERATION_ALLOCATE) {
        ++self->profile_allocate_calls;
        self->profile_allocated_byte_count += byte_count;
    } else if (operation == OPERATION_FREE) {
        ++self->profile_free_calls;
        self->profile_freed_byte_count += byte_count;
    }

#ifdef CHECK_DOUBLE_FREE
#ifdef BSAL_DEBUGGER_ENABLE_ASSERT
    if (!(self->profile_allocate_calls >= self->profile_free_calls)) {
        bsal_memory_pool_examine(self);
    }
#endif
    BSAL_DEBUGGER_ASSERT(self->profile_allocate_calls >= self->profile_free_calls);
#endif
}

int bsal_memory_pool_has_leaks(struct bsal_memory_pool *self)
{
#ifdef CHECK_LEAKS
    return self->profile_allocate_calls != self->profile_free_calls;
#else
    return 0;
#endif
}

void bsal_memory_pool_begin(struct bsal_memory_pool *self, struct bsal_memory_pool_state *state)
{
    state->test_profile_allocate_calls = self->profile_allocate_calls;
    state->test_profile_free_calls = self->profile_free_calls;
}

void bsal_memory_pool_end(struct bsal_memory_pool *self, struct bsal_memory_pool_state *state,
                const char *name, const char *function, const char *file, int line)
{
    int allocate_calls;
    int free_calls;

    allocate_calls = self->profile_allocate_calls - state->test_profile_allocate_calls;
    free_calls = self->profile_free_calls - state->test_profile_free_calls;

    if (allocate_calls != free_calls) {
        printf("Error, saved pool state \"%s\" (%s %s %d) reveals leaks: allocate_calls %d free_calls %d\n",
                        name, function, file, line, allocate_calls, free_calls);
    }

    BSAL_DEBUGGER_ASSERT(allocate_calls == free_calls);
}
