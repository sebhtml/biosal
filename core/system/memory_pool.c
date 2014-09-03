
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

/*
#define TRACK_EXTERNAL_OPERATIONS
*/
/*
 * Enable the tracking for allocations smaller or equal to block_size
 */
#define FLAG_ENABLE_TRACKING 0

/*
 * Disable the memory pool.
 */
#define FLAG_DISABLED 1

/*
 * Enable segment normalization. Any incoming size is then transformed
 * before being passed to bsal_memory_pool_allocate_private.
 */
#define FLAG_ENABLE_SEGMENT_NORMALIZATION 2

/*
 * Align the size on a boundary.
 */
#define FLAG_ALIGN 3

/*
 * FLAG_EPHEMERAL normalizes any allocation larger than block_size.
 */
#define FLAG_EPHEMERAL 4

/*
 * This disable the block subsystem.
 */
#define FLAG_DISABLED_BLOCK_ALLOCATION 5

/*
 * Different code paths.
 */
#define CODE_PATH_SMALL_REUSE 0
#define CODE_PATH_SMALL_NEW 1
#define CODE_PATH_EXTERNAL_REUSE 2
#define CODE_PATH_EXTERNAL_NEW 3
#define CODE_PATH_DISABLED_NEW 4

void bsal_memory_pool_init(struct bsal_memory_pool *self, size_t block_size)
{
    bsal_memory_pool_set_name(self, BSAL_MEMORY_POOL_NAME_NONE);
    bsal_map_init(&self->recycle_bin, sizeof(size_t), sizeof(struct bsal_fast_queue));
    bsal_map_init(&self->allocated_blocks, sizeof(void *), sizeof(size_t));

    bsal_map_init(&self->external_recycle_bin, sizeof(size_t), sizeof(struct bsal_fast_queue));
    bsal_map_init(&self->external_allocated_blocks, sizeof(void *), sizeof(size_t));

    self->current_block = NULL;

    bsal_fast_queue_init(&self->dried_blocks, sizeof(struct bsal_memory_block *));
    bsal_fast_queue_init(&self->ready_blocks, sizeof(struct bsal_memory_block *));

    self->block_size = block_size;

    /*
     * Configure flags
     */
    self->flags = 0;
    bsal_bitmap_set_bit_uint32_t(&self->flags, FLAG_ENABLE_TRACKING);
    bsal_bitmap_clear_bit_uint32_t(&self->flags, FLAG_DISABLED);
    bsal_bitmap_clear_bit_uint32_t(&self->flags, FLAG_DISABLED_BLOCK_ALLOCATION);
    bsal_bitmap_clear_bit_uint32_t(&self->flags, FLAG_ENABLE_SEGMENT_NORMALIZATION);
    bsal_bitmap_clear_bit_uint32_t(&self->flags, FLAG_ALIGN);
    bsal_bitmap_clear_bit_uint32_t(&self->flags, FLAG_EPHEMERAL);
}

void bsal_memory_pool_destroy(struct bsal_memory_pool *self)
{
    struct bsal_fast_queue *queue;
    struct bsal_map_iterator iterator;
    struct bsal_memory_block *block;
    void *pointer;

    /* destroy recycled objects
     */
    bsal_map_iterator_init(&iterator, &self->recycle_bin);

    while (bsal_map_iterator_has_next(&iterator)) {
        bsal_map_iterator_next(&iterator, NULL, (void **)&queue);

        bsal_fast_queue_destroy(queue);
    }
    bsal_map_iterator_destroy(&iterator);
    bsal_map_destroy(&self->recycle_bin);

    /* destroy allocated blocks */

#ifdef TRACK_MEMORY_LEAKS
#ifdef BSAL_DEBUGGER_ENABLE_ASSERT
    if (bsal_map_size(&self->allocated_blocks) > 0) {
        printf("Error, %d allocated blocks not freed\n",
                        (int)bsal_map_size(&self->allocated_blocks));
    }
#endif
    BSAL_DEBUGGER_ASSERT(bsal_map_empty(&self->allocated_blocks));
#endif

    bsal_map_destroy(&self->allocated_blocks);

    /*
     * Destroy external segments.
     */
    bsal_map_iterator_init(&iterator, &self->external_recycle_bin);

    while (bsal_map_iterator_has_next(&iterator)) {
        bsal_map_iterator_next(&iterator, NULL, (void **)&queue);

        while (bsal_fast_queue_dequeue(queue, &pointer)) {
            bsal_memory_free(pointer);
        }
        bsal_fast_queue_destroy(queue);
    }

    bsal_map_iterator_destroy(&iterator);

#ifdef TRACK_MEMORY_LEAKS
#ifdef BSAL_DEBUGGER_ENABLE_ASSERT
    if (bsal_map_size(&self->external_allocated_blocks) > 0) {
        printf("Error, %d external allocated blocks not freed\n",
                        (int)bsal_map_size(&self->external_allocated_blocks));
    }
#endif

    BSAL_DEBUGGER_ASSERT(bsal_map_empty(&self->external_allocated_blocks));
#endif

    bsal_map_destroy(&self->external_allocated_blocks);

    /*
     * destroy dried blocks
     */
    while (bsal_fast_queue_dequeue(&self->dried_blocks, &block)) {
        bsal_memory_block_destroy(block);
        bsal_memory_free(block);
    }
    bsal_fast_queue_destroy(&self->dried_blocks);

    /* destroy ready blocks
     */
    while (bsal_fast_queue_dequeue(&self->ready_blocks, &block)) {
        bsal_memory_block_destroy(block);
        bsal_memory_free(block);
    }
    bsal_fast_queue_destroy(&self->ready_blocks);

    /* destroy the current block
     */
    if (self->current_block != NULL) {
        bsal_memory_block_destroy(self->current_block);
        bsal_memory_free(self->current_block);
        self->current_block = NULL;
    }
}

void *bsal_memory_pool_allocate(struct bsal_memory_pool *self, size_t size)
{
    void *pointer;
    size_t new_size;
    int normalize;
    int path;

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

    pointer = bsal_memory_pool_allocate_private(self, size, &path);

#ifdef DEBUG_MEMORY_LEAK_2014_09_02
    if (size == 8388608) {
        printf("memory_pool/%d bsal_memory_pool_allocate size= %zu path= %d external_allocated_blocks: %d allocated_blocks: %d\n",
                        self->name,
                    size, path, (int)bsal_map_size(&self->external_allocated_blocks),
                    (int)bsal_map_size(&self->allocated_blocks));
    }
#endif

    if (pointer == NULL) {
        printf("Error, requested %zu bytes, returned pointer is NULL (code path: %d, block_size %zu)\n",
                        size, path, self->block_size);

        bsal_tracer_print_stack_backtrace();
        printf("used / total -> %" PRIu64 " / %" PRIu64  "\n",
                        bsal_memory_get_utilized_byte_count(),
                        bsal_memory_get_total_byte_count());

        exit(1);
    }

    return pointer;
}

void *bsal_memory_pool_allocate_private(struct bsal_memory_pool *self, size_t size, int *path)
{
    struct bsal_fast_queue *queue;
    void *pointer;
    size_t *bucket;

    if (size == 0) {
        return NULL;
    }

    if (self == NULL || bsal_bitmap_get_bit_uint32_t(&self->flags, FLAG_DISABLED)) {
        return bsal_memory_allocate(size);
    }

    BSAL_DEBUGGER_ASSERT(self != NULL);

    /*
     * Check if this large size is available in the external recycle bin.
     */
    if (size > self->block_size) {

        queue = bsal_map_get(&self->external_recycle_bin, &size);

        if (queue != NULL
                        && bsal_fast_queue_dequeue(queue, &pointer)) {

            *path = CODE_PATH_EXTERNAL_REUSE;
            bucket = bsal_map_add(&self->external_allocated_blocks, &pointer);
            *bucket = size;

            return pointer;
        }
    }

    /*
     * First, check if the size is larger than the maximum size.
     * If memory blocks can not fulfil the need, use the memory system
     * directly.
     *
     * If FLAG_DISABLED_BLOCK_ALLOCATION is set, then don't use block allocation
     * at all.
     */

    if (size > self->block_size
           || bsal_bitmap_get_bit_uint32_t(&self->flags, FLAG_DISABLED_BLOCK_ALLOCATION)) {

        pointer = bsal_memory_allocate(size);

        BSAL_DEBUGGER_ASSERT(pointer != NULL);

        BSAL_DEBUGGER_ASSERT(path != NULL);
        *path = CODE_PATH_EXTERNAL_NEW;

        BSAL_DEBUGGER_ASSERT(self != NULL);

#ifdef BSAL_DEBUGGER_ENABLE_ASSERT
        if (bsal_map_get(&self->external_allocated_blocks, &pointer) != NULL) {
            bucket = bsal_map_get(&self->external_allocated_blocks, &pointer);
            printf("Error, pool/%d pointer %p found size %zu block_size %zu\n",
                            self->name, pointer, *bucket, self->block_size);
        }
#endif
        BSAL_DEBUGGER_ASSERT(bsal_map_get(&self->external_allocated_blocks, &pointer) == NULL);

        bucket = bsal_map_add(&self->external_allocated_blocks, &pointer);

        BSAL_DEBUGGER_ASSERT(bucket != NULL);

        *bucket = size;

        return pointer;
    }

    /*
     * Make sure that block allocation is not disabled.
     */
    BSAL_DEBUGGER_ASSERT(!bsal_bitmap_get_bit_uint32_t(&self->flags, FLAG_DISABLED_BLOCK_ALLOCATION));

    /*
     * Look out for a small piece.
     */
    queue = NULL;

    if (bsal_bitmap_get_bit_uint32_t(&self->flags, FLAG_ENABLE_TRACKING)) {
        queue = bsal_map_get(&self->recycle_bin, &size);
    }

    /* recycling is good for the environment
     */
    if (queue != NULL && bsal_fast_queue_dequeue(queue, &pointer)) {

        if (bsal_bitmap_get_bit_uint32_t(&self->flags, FLAG_ENABLE_TRACKING)) {
            bsal_map_add_value(&self->allocated_blocks, &pointer, &size);
        }

#ifdef BSAL_MEMORY_POOL_DISCARD_EMPTY_QUEUES
        if (bsal_fast_queue_empty(queue)) {
            bsal_fast_queue_destroy(queue);
            bsal_map_delete(&self->recycle_bin, &size);
        }
#endif

        *path = CODE_PATH_SMALL_REUSE;
        return pointer;
    }

    if (self->current_block == NULL) {

        bsal_memory_pool_add_block(self);
    }

    pointer = bsal_memory_block_allocate(self->current_block, size);

    /* the current block is exausted...
     */
    if (pointer == NULL) {
        bsal_fast_queue_enqueue(&self->dried_blocks, &self->current_block);
        self->current_block = NULL;

        bsal_memory_pool_add_block(self);

        pointer = bsal_memory_block_allocate(self->current_block, size);
    }

    if (bsal_bitmap_get_bit_uint32_t(&self->flags, FLAG_ENABLE_TRACKING)) {
        bsal_map_add_value(&self->allocated_blocks, &pointer, &size);
    }

    *path = CODE_PATH_SMALL_NEW;
    return pointer;
}

void bsal_memory_pool_add_block(struct bsal_memory_pool *self)
{
    /* Try to pick a block in the ready block list.
     * Otherwise, create one on-demand today.
     */
    if (!bsal_fast_queue_dequeue(&self->ready_blocks, &self->current_block)) {
        self->current_block = bsal_memory_allocate(sizeof(struct bsal_memory_block));
        bsal_memory_block_init(self->current_block, self->block_size);
    }
}

void bsal_memory_pool_free(struct bsal_memory_pool *self, void *pointer)
{
    struct bsal_fast_queue *queue;
    size_t size;
    size_t *bucket;

    if (pointer == NULL) {
        return;
    }

    if (self == NULL || bsal_bitmap_get_bit_uint32_t(&self->flags, FLAG_DISABLED)) {
        bsal_memory_free(pointer);
        return;
    }

    /* Verify if the pointer is a large block not managed by one of the memory
     * blocks
     */

    bucket = bsal_map_get(&self->external_allocated_blocks, &pointer);

    if (bucket != NULL) {

#if 0
        if (self->name == BSAL_MEMORY_POOL_NAME_NODE_INBOUND) {
            printf("DEBUG free %p found bucket\n", pointer);
        }
#endif

        size = *bucket;

#ifdef DEBUG_MEMORY_LEAK_2014_09_02
        if (size == 8388608) {
            printf("memory_pool/%d Freeing point %zu\n", self->name, size);
        }
#endif

        BSAL_DEBUGGER_ASSERT(bsal_map_get(&self->external_allocated_blocks, &pointer) != NULL);

        /*
         * There is a bug in map_delete...
         * https://github.com/GeneAssembly/biosal/issues/641
         */
        bsal_map_delete(&self->external_allocated_blocks, &pointer);
        bsal_map_delete(&self->external_allocated_blocks, &pointer);

#ifdef BSAL_DEBUGGER_ASSERT
        if (bsal_map_get(&self->external_allocated_blocks, &pointer) != NULL) {
            printf("Error, pool/%d pointer %p is still registered after deletion (%d items)\n",
                            self->name, pointer,
                            (int)bsal_map_size(&self->external_allocated_blocks));
        }
#endif
        BSAL_DEBUGGER_ASSERT(bsal_map_get(&self->external_allocated_blocks, &pointer) == NULL);

        bsal_memory_pool_recycle_external_segment(self, size, pointer);

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
        bsal_fast_queue_init(queue, sizeof(void *));
    }

    bsal_fast_queue_enqueue(queue, &pointer);

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
    int queue_size;
    void *pointer;
    size_t segment_length;
    struct bsal_map_iterator iterator;

    /*
     * Reset the current block
     */
    if (self->current_block != NULL) {
        bsal_memory_block_free_all(self->current_block);
    }

    /*
     * Reset all ready blocks
     */
    queue_size = bsal_fast_queue_size(&self->ready_blocks);
    i = 0;
    while (i < queue_size
                   && bsal_fast_queue_dequeue(&self->ready_blocks, &block)) {
        bsal_memory_block_free_all(block);
        bsal_fast_queue_enqueue(&self->ready_blocks, &block);

        i++;
    }

    /*
     * Reset all dried blocks
     */
    while (bsal_fast_queue_dequeue(&self->dried_blocks, &block)) {
        bsal_memory_block_free_all(block);
        bsal_fast_queue_enqueue(&self->ready_blocks, &block);
    }

    /*
     * Reset current structures.
     */
    if (bsal_bitmap_get_bit_uint32_t(&self->flags, FLAG_ENABLE_TRACKING)) {
        bsal_map_clear(&self->allocated_blocks);
        bsal_map_clear(&self->recycle_bin);
    }

    /*
     * Put all large blocks in the recycle bin for
     * external blocks.
     */

    bsal_map_iterator_init(&iterator, &self->external_allocated_blocks);

    while (bsal_map_iterator_get_next_key_and_value(&iterator, &pointer, &segment_length)) {
        bsal_memory_pool_recycle_external_segment(self, segment_length, pointer);
    }

    bsal_map_iterator_destroy(&iterator);

    bsal_map_clear(&self->external_allocated_blocks);
}

void bsal_memory_pool_recycle_external_segment(struct bsal_memory_pool *self, size_t size,
                void *pointer)
{
#ifdef TRACK_EXTERNAL_OPERATIONS
    struct bsal_fast_queue *queue;

    queue = bsal_map_get(&self->external_recycle_bin, &size);

    if (queue == NULL) {
        queue = bsal_map_add(&self->external_recycle_bin, &size);
        bsal_fast_queue_init(queue, sizeof(void *));
    }

    BSAL_DEBUGGER_ASSERT(queue != NULL);
    BSAL_DEBUGGER_ASSERT(pointer != NULL);

    bsal_fast_queue_enqueue(queue, &pointer);
#else
    bsal_memory_free(pointer);
#endif
}

void bsal_memory_pool_disable(struct bsal_memory_pool *self)
{
    bsal_bitmap_set_bit_uint32_t(&self->flags, FLAG_DISABLED);
}

void bsal_memory_pool_disable_block_allocation(struct bsal_memory_pool *self)
{
    bsal_bitmap_set_bit_uint32_t(&self->flags, FLAG_DISABLED_BLOCK_ALLOCATION);
}

void bsal_memory_pool_print(struct bsal_memory_pool *self)
{
    int block_count;
    uint64_t byte_count;

    block_count = 0;

    if (self->current_block != NULL) {
        ++block_count;
    }

    block_count += bsal_fast_queue_size(&self->dried_blocks);
    block_count += bsal_fast_queue_size(&self->ready_blocks);

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
    self->name = name;
}
