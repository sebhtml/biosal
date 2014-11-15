
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
 * Examine memory pool on destruct().
 */
/*
#define CORE_MEMORY_POOL_EXAMINE
*/

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

#define MEMORY_MEMORY_POOL_NULL_SELF 0xc170626e

/*
 * Private
 */

static void core_memory_pool_add_block(struct core_memory_pool *self);
static void core_memory_pool_set_name(struct core_memory_pool *self, int name);
static void *core_memory_pool_allocate_private(struct core_memory_pool *self, size_t size);
static void core_memory_pool_free_private(struct core_memory_pool *self, void *pointer);

void core_memory_pool_print_allocated_blocks(struct core_memory_pool *self);

void core_memory_pool_init(struct core_memory_pool *self, int block_size, int name)
{
    core_map_init(&self->recycle_bin, sizeof(size_t), sizeof(struct core_queue));
    core_map_init(&self->allocated_blocks, sizeof(void *), sizeof(size_t));
    core_set_init(&self->large_blocks, sizeof(void *));

    self->current_block = NULL;
    core_memory_pool_set_name(self, name);

    core_queue_init(&self->dried_blocks, sizeof(struct core_memory_block *));
    core_queue_init(&self->ready_blocks, sizeof(struct core_memory_block *));

    self->block_size = block_size;

    /*
     * Configure flags
     */
    CORE_BITMAP_CLEAR(self->flags);

    CORE_BITMAP_SET_BIT(self->flags, FLAG_ENABLE_TRACKING);

    CORE_BITMAP_CLEAR_BIT(self->flags, FLAG_DISABLED);
    CORE_BITMAP_CLEAR_BIT(self->flags, FLAG_ENABLE_SEGMENT_NORMALIZATION);
    CORE_BITMAP_CLEAR_BIT(self->flags, FLAG_ALIGN);
    CORE_BITMAP_CLEAR_BIT(self->flags, FLAG_EPHEMERAL);

    self->profile_allocated_byte_count = 0;
    self->profile_freed_byte_count = 0;
    self->profile_allocate_calls = 0;
    self->profile_free_calls = 0;

    self->final = 0;
}

void core_memory_pool_destroy(struct core_memory_pool *self)
{
    struct core_queue *queue;
    struct core_map_iterator iterator;
    struct core_memory_block *block;

    self->final = 1;

#ifdef CORE_MEMORY_POOL_EXAMINE
    core_memory_pool_examine(self);
#endif

    if (core_memory_pool_has_leaks(self)) {
        printf("Error, memory leak detected.\n");
        core_memory_pool_examine(self);
    }

#ifdef CORE_MEMORY_POOL_FIND_LEAKS
#ifdef CORE_DEBUGGER_ASSERT_ENABLED
    CORE_DEBUGGER_ASSERT(!core_memory_pool_has_leaks(self));
#endif
#endif

    /* destroy recycled objects
     */
    core_map_iterator_init(&iterator, &self->recycle_bin);

    while (core_map_iterator_has_next(&iterator)) {
        core_map_iterator_next(&iterator, NULL, (void **)&queue);

        core_queue_destroy(queue);
    }
    core_map_iterator_destroy(&iterator);
    core_map_destroy(&self->recycle_bin);

    /* destroy allocated blocks */
    core_map_destroy(&self->allocated_blocks);

    /* destroy dried blocks
     */
    while (core_queue_dequeue(&self->dried_blocks, &block)) {
        core_memory_block_destroy(block);
        core_memory_free(block, self->name);
    }
    core_queue_destroy(&self->dried_blocks);

    /* destroy ready blocks
     */
    while (core_queue_dequeue(&self->ready_blocks, &block)) {
        core_memory_block_destroy(block);
        core_memory_free(block, self->name);
    }
    core_queue_destroy(&self->ready_blocks);

    /* destroy the current block
     */
    if (self->current_block != NULL) {
        core_memory_block_destroy(self->current_block);
        core_memory_free(self->current_block, self->name);
        self->current_block = NULL;
    }

    core_set_destroy(&self->large_blocks);
}

void *core_memory_pool_allocate(struct core_memory_pool *self, size_t size)
{
    void *pointer;
    size_t new_size;
    int normalize;

    CORE_DEBUGGER_ASSERT(size > 0);

    if (self == NULL) {
        return core_memory_allocate(size, MEMORY_MEMORY_POOL_NULL_SELF);
    }

#ifdef CORE_DEBUGGER_ASSERT_ENABLED
    if (size < CORE_MEMORY_MINIMUM) {
        printf("Error: too low %zu\n", size);
    }
    if (size > CORE_MEMORY_MAXIMUM) {
        printf("Error: too high %zu\n", size);
    }
#endif

    CORE_DEBUGGER_ASSERT(size >= CORE_MEMORY_MINIMUM);
    CORE_DEBUGGER_ASSERT(size <= CORE_MEMORY_MAXIMUM);

    normalize = 0;

    /*
     * Normalize the length of the segment to be a power of 2
     * if the flag FLAG_ENABLE_SEGMENT_NORMALIZATION is set.
     */
    if (CORE_BITMAP_GET_BIT(self->flags, FLAG_ENABLE_SEGMENT_NORMALIZATION)) {
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
              && CORE_BITMAP_GET_BIT(self->flags, FLAG_EPHEMERAL)) {
        normalize = 1;
    }

    if (normalize) {

        /*
         * The Blue Gene/Q seems to prefer powers of 2
         * otherwise fragmentation makes the system run out of memory.
         */
        new_size = core_memory_normalize_segment_length_power_of_2(size);
            /*
        new_size = core_memory_normalize_segment_length_page_size(size);
        */
#if 0
        printf("NORMALIZE %zu -> %zu\n", size, new_size);
#endif
        size = new_size;
    }

    CORE_DEBUGGER_ASSERT(size >= CORE_MEMORY_MINIMUM);
    CORE_DEBUGGER_ASSERT(size <= CORE_MEMORY_MAXIMUM);

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
    if (CORE_BITMAP_GET_BIT(self->flags, FLAG_ALIGN)) {

        new_size = core_memory_align(size);

        size = new_size;
    }

    CORE_DEBUGGER_ASSERT(size >= CORE_MEMORY_MINIMUM);
    CORE_DEBUGGER_ASSERT(size <= CORE_MEMORY_MAXIMUM);

    pointer = core_memory_pool_allocate_private(self, size);

    if (pointer == NULL) {
        printf("Error, requested %zu bytes, returned pointer is NULL\n",
                        size);

        core_tracer_print_stack_backtrace();

        exit(1);
    }

    if (CORE_BITMAP_GET_BIT(self->flags, FLAG_ENABLE_TRACKING)) {

#ifdef CORE_DEBUGGER_ASSERT_ENABLED
        if (core_map_get(&self->allocated_blocks, &pointer) != NULL) {
            printf("Error, pointer %p is already in use, %zu bytes (on record: %zu bytes)\n",
                            pointer, size,
                            *(size_t *)core_map_get(&self->allocated_blocks, &pointer));
        }
#endif

        /*
         * Make sure that it is not allocated already.
         */
        CORE_DEBUGGER_ASSERT(core_map_get(&self->allocated_blocks, &pointer) == NULL);

        core_map_add_value(&self->allocated_blocks, &pointer, &size);

        CORE_DEBUGGER_ASSERT(core_map_get(&self->allocated_blocks, &pointer) != NULL);
    }

    core_memory_pool_profile(self, OPERATION_ALLOCATE, size);

#ifdef DEBUG_MEMORY_POOL_ALLOCATE
    printf("DEBUG pool_allocate name %d self %p pointer %p size %zu\n",
                    self->name, (void *)self, pointer, size);
#endif

    return pointer;
}

static void *core_memory_pool_allocate_private(struct core_memory_pool *self, size_t size)
{
    struct core_queue *queue;
    void *pointer;

    if (size == 0) {
        return NULL;
    }

    if (CORE_BITMAP_GET_BIT(self->flags, FLAG_DISABLED)) {
        return core_memory_allocate(size, self->name);
    }

    /*
     * First, check if the size is larger than the maximum size.
     * If memory blocks can not fulfil the need, use the memory system
     * directly.
     */

    if (size >= self->block_size) {
        pointer = core_memory_allocate(size, self->name);

        core_set_add(&self->large_blocks, &pointer);

        return pointer;
    }

    queue = NULL;

    if (CORE_BITMAP_GET_BIT(self->flags, FLAG_ENABLE_TRACKING)) {
        queue = core_map_get(&self->recycle_bin, &size);
    }

    /* recycling is good for the environment
     */
    if (queue != NULL && core_queue_dequeue(queue, &pointer)) {

#ifdef CORE_MEMORY_POOL_DISCARD_EMPTY_QUEUES
        if (core_queue_empty(queue)) {
            core_queue_destroy(queue);
            core_map_delete(&self->recycle_bin, &size);
        }
#endif

#ifdef DEBUG_MEMORY_POOL_ALLOCATE
        printf("DEBUG pool_allocate_private from recycle_bin size %zu pointer %p\n",
                        size, pointer);
#endif

        return pointer;
    }

    if (self->current_block == NULL) {

        core_memory_pool_add_block(self);
    }

    pointer = core_memory_block_allocate(self->current_block, size);

    /* the current block is exausted...
     */
    if (pointer == NULL) {
        core_queue_enqueue(&self->dried_blocks, &self->current_block);
        self->current_block = NULL;

        core_memory_pool_add_block(self);

        pointer = core_memory_block_allocate(self->current_block, size);

#ifdef DEBUG_MEMORY_POOL_ALLOCATE
        printf("DEBUG pool_allocate_private from block size %zu pointer %p\n",
                        size, pointer);
#endif
    }

    return pointer;
}

static void core_memory_pool_add_block(struct core_memory_pool *self)
{
    /* Try to pick a block in the ready block list.
     * Otherwise, create one on-demand today.
     */
    if (!core_queue_dequeue(&self->ready_blocks, &self->current_block)) {
        self->current_block = core_memory_allocate(sizeof(struct core_memory_block), self->name);
        core_memory_block_init(self->current_block, self->block_size);
    }
}

int core_memory_pool_free(struct core_memory_pool *self, void *pointer)
{
    size_t size;
    int value;

    CORE_DEBUGGER_ASSERT(pointer != NULL);

    if (self == NULL) {
        core_memory_free(pointer, MEMORY_MEMORY_POOL_NULL_SELF);
        return 1;
    }

#ifdef DEBUG_MEMORY_POOL_FREE
    printf("pool_free self= %p name= %d pointer= %p\n", (void *)self, self->name, pointer);
#endif

    size = 0;
    value = 0;

    /*
     * find out the actual size.
     */
    if (core_map_get_value(&self->allocated_blocks, &pointer, &size)) {

        core_memory_pool_free_private(self, pointer);
/*
#ifdef CORE_DEBUGGER_ASSERT_ENABLED
        if (!(self->profile_allocate_calls - self->profile_free_calls) ==
                                                (int)core_map_size(&self->allocated_blocks)) {
            printf("Error: balance is incorrect, profile_allocate_calls %d profile_free_calls %d allocated_blocks.size %d\n",
                self->profile_allocate_calls, self->profile_free_calls,
                (int)core_map_size(&self->allocated_blocks));
        }
#endif

        CORE_DEBUGGER_ASSERT((self->profile_allocate_calls - self->profile_free_calls) ==
                        (int)core_map_size(&self->allocated_blocks));
*/
#ifdef CORE_DEBUGGER_ASSERT_ENABLED_
        void *before = core_map_get(&self->allocated_blocks, &pointer);
#endif
        core_map_delete(&self->allocated_blocks, &pointer);
/*
        printf("DEBUG_MEMORY_POOL freed %zu bytes pointer %p\n", size, pointer);
        */

#ifdef CORE_DEBUGGER_ASSERT_ENABLED
        void *after = core_map_get(&self->allocated_blocks, &pointer);

        if (after != NULL) {
                /*
            printf("Error, pointer is still here: %p, deleted bucket %p other bucket %p\n",
                            pointer, before, after);
                            */

            core_memory_pool_print_allocated_blocks(self);
        }
#endif

        /*
         * Make sure that it is gone.
         */
        CORE_DEBUGGER_ASSERT(core_map_get(&self->allocated_blocks, &pointer) == NULL);

        value = 1;
    }

    /*
     * If FLAG_ENABLE_TRACKING is set, verify that the current memory pool
     * manages this pointer.
     */
#ifdef CORE_DEBUGGER_ASSERT_ENABLED
    if (CORE_BITMAP_GET_BIT(self->flags, FLAG_ENABLE_TRACKING)) {
        if (!value)
            printf("Error, memory pool (self= %p) 0x%x does not manage buffer %p\n",
                        (void *)self, self->name, pointer);

        CORE_DEBUGGER_ASSERT(value);
    }
#endif

    /*
     * If the FLAG_ENABLE_TRACKING bit is 0, then the map "allocated_blocks"
     * never contains anything (even for large segments). Thus, the if block
     * just above this comment never does anything when FLAG_ENABLE_TRACKING
     * is 0.
     *
     * The solution, when FLAG_ENABLE_TRACKING is cleared, is to send every
     * allocation through the core_memory_pool_free_private() code path. In
     * this function, large_blocks are still tracked even if
     * FLAG_ENABLE_TRACKING is 0.
     *
     * This is to avoid errors when memory blocks of the memory pool can not
     * fulfill allocations that are too large.
     *
     * This memory leak was affecting performance as well, when the ephemeral
     * memory pool of any worker allocates large segments of memory because
     * these were not freed anymore.
     *
     * The call to core_memory_pool_free_private() below call fixes the
     * regression added in commit 153eea9 ("thorium_node: use message type to
     * select pool to free buffer").
     *
     * Link: https://github.com/GeneAssembly/biosal/issues/788#issuecomment-62002496
     */
    if (!CORE_BITMAP_GET_BIT(self->flags, FLAG_ENABLE_TRACKING)) {
        core_memory_pool_free_private(self, pointer);
    }

    /*
     * Profile the call. This is done even when FLAG_ENABLE_TRACKING
     * is not set.
     */
    core_memory_pool_profile(self, OPERATION_FREE, size);

    return value;
}

static void core_memory_pool_free_private(struct core_memory_pool *self, void *pointer)
{
    struct core_queue *queue;
    size_t size;

    if (CORE_BITMAP_GET_BIT(self->flags, FLAG_DISABLED)) {
        core_memory_free(pointer, self->name);
        return;
    }

    /* Verify if the pointer is a large block not managed by one of the memory
     * blocks
     */
    if (core_set_find(&self->large_blocks, &pointer)) {

        core_memory_free(pointer, self->name);
        core_set_delete(&self->large_blocks, &pointer);
        return;
    }

    /*
     * Return immediately if memory allocation tracking is disabled.
     * For example, the ephemeral memory component of a worker
     * disable tracking (flag FLAG_ENABLE_TRACKING = 0). To free memory,
     * for the ephemeral memory, core_memory_pool_free_all is
     * used.
     */
    if (!CORE_BITMAP_GET_BIT(self->flags, FLAG_ENABLE_TRACKING)) {
        return;
    }

    /*
     * This was not allocated by this pool.
     */
    if (!core_map_get_value(&self->allocated_blocks, &pointer, &size)) {
        return;
    }

    queue = core_map_get(&self->recycle_bin, &size);

    if (queue == NULL) {
        queue = core_map_add(&self->recycle_bin, &size);
        core_queue_init(queue, sizeof(void *));
    }

    core_queue_enqueue(queue, &pointer);
}

void core_memory_pool_disable_tracking(struct core_memory_pool *self)
{
    CORE_BITMAP_CLEAR_BIT(self->flags, FLAG_ENABLE_TRACKING);
}

void core_memory_pool_enable_normalization(struct core_memory_pool *self)
{
    CORE_BITMAP_SET_BIT(self->flags, FLAG_ENABLE_SEGMENT_NORMALIZATION);
}

void core_memory_pool_disable_normalization(struct core_memory_pool *self)
{
    CORE_BITMAP_CLEAR_BIT(self->flags, FLAG_ENABLE_SEGMENT_NORMALIZATION);
}

void core_memory_pool_enable_alignment(struct core_memory_pool *self)
{
    CORE_BITMAP_SET_BIT(self->flags, FLAG_ALIGN);
}

void core_memory_pool_disable_alignment(struct core_memory_pool *self)
{
    CORE_BITMAP_CLEAR_BIT(self->flags, FLAG_ALIGN);
}

void core_memory_pool_enable_tracking(struct core_memory_pool *self)
{
    CORE_BITMAP_SET_BIT(self->flags, FLAG_ENABLE_TRACKING);
}

void core_memory_pool_free_all(struct core_memory_pool *self)
{
    struct core_memory_block *block;
    int i;
    int size;

#ifdef CORE_DEBUGGER_ASSERT_ENABLED
    if (core_memory_pool_has_leaks(self)) {
        core_memory_pool_examine(self);
    }

    CORE_DEBUGGER_ASSERT(!core_memory_pool_has_leaks(self));
#endif

    /*
     * Reset the current block
     */
    if (self->current_block != NULL) {
        core_memory_block_free_all(self->current_block);
    }

    /*
     * Reset all ready blocks
     */
    size = core_queue_size(&self->ready_blocks);
    i = 0;
    while (i < size
                   && core_queue_dequeue(&self->ready_blocks, &block)) {
        core_memory_block_free_all(block);
        core_queue_enqueue(&self->ready_blocks, &block);

        i++;
    }

    /*
     * Reset all dried blocks
     */
    while (core_queue_dequeue(&self->dried_blocks, &block)) {
        core_memory_block_free_all(block);
        core_queue_enqueue(&self->ready_blocks, &block);
    }

    /*
     * Reset current structures.
     */
    if (CORE_BITMAP_GET_BIT(self->flags, FLAG_ENABLE_TRACKING)) {
        core_map_clear(&self->allocated_blocks);
        core_map_clear(&self->recycle_bin);
    }

    if (!CORE_BITMAP_GET_BIT(self->flags, FLAG_DISABLED)) {
        core_set_clear(&self->large_blocks);
    }
}

void core_memory_pool_disable(struct core_memory_pool *self)
{
    CORE_BITMAP_SET_BIT(self->flags, FLAG_DISABLED);
}

void core_memory_pool_print(struct core_memory_pool *self)
{
    int block_count;
    uint64_t byte_count;

    block_count = 0;

    if (self->current_block != NULL) {
        ++block_count;
    }

    block_count += core_queue_size(&self->dried_blocks);
    block_count += core_queue_size(&self->ready_blocks);

    byte_count = (uint64_t)block_count * (uint64_t)self->block_size;

    printf("PRINT POOL Name= 0x%x memory_pool BlockSize: %d BlockCount: %d ByteCount: %" PRIu64 "\n",
                    self->name,
                    (int)self->block_size,
                    block_count,
                    byte_count);
}

void core_memory_pool_enable_ephemeral_mode(struct core_memory_pool *self)
{
    CORE_BITMAP_SET_BIT(self->flags, FLAG_EPHEMERAL);
}

static void core_memory_pool_set_name(struct core_memory_pool *self, int name)
{
    self->name = name;
}

void core_memory_pool_examine(struct core_memory_pool *self)
{
    int balance;

    balance = self->profile_allocate_calls - self->profile_free_calls;

    printf("DEBUG_POOL Name= 0x%x Self=%p Result= %s"
                    " AllocatedPointerCount= %d (%d - %d)"
                    " AllocatedByteCount= %" PRIu64 " (%" PRIu64 " - %" PRIu64 ")"
                    "\n",

                    self->name,
                    (void *)self,
                    (self->final == 1  ? (balance == 0 ? "PASSED" : "FAILED") : "-"),
                    balance,
                    self->profile_allocate_calls, self->profile_free_calls,

                    self->profile_allocated_byte_count - self->profile_freed_byte_count,
                    self->profile_allocated_byte_count, self->profile_freed_byte_count);

#if 0
    core_memory_pool_print(self);
#endif
}

void core_memory_pool_profile(struct core_memory_pool *self, int operation, size_t byte_count)
{
    if (operation == OPERATION_ALLOCATE) {
        ++self->profile_allocate_calls;
        self->profile_allocated_byte_count += byte_count;
    } else if (operation == OPERATION_FREE) {
        ++self->profile_free_calls;
        self->profile_freed_byte_count += byte_count;
    }

#ifdef CORE_DEBUGGER_CHECK_DOUBLE_FREE_IN_POOL
#ifdef CORE_DEBUGGER_ASSERT_ENABLED
    if (!(self->profile_allocate_calls >= self->profile_free_calls)) {
        core_memory_pool_examine(self);
    }
#endif
    CORE_DEBUGGER_ASSERT(self->profile_allocate_calls >= self->profile_free_calls);
#endif
}

int core_memory_pool_has_leaks(struct core_memory_pool *self)
{
    int balance;

    balance = self->profile_allocate_calls != self->profile_free_calls;

    if (balance != 0)
        return 1;

    if (!core_set_empty(&self->large_blocks))
        return 1;

    if (!core_map_empty(&self->allocated_blocks))
        return 1;

    return 0;
}

void core_memory_pool_begin(struct core_memory_pool *self, struct core_memory_pool_state *state)
{
    state->test_profile_allocate_calls = self->profile_allocate_calls;
    state->test_profile_free_calls = self->profile_free_calls;
}

void core_memory_pool_end(struct core_memory_pool *self, struct core_memory_pool_state *state,
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

    CORE_DEBUGGER_ASSERT(allocate_calls == free_calls);
}

int core_memory_pool_has_double_free(struct core_memory_pool *self)
{
    return self->profile_allocate_calls < self->profile_free_calls;
}

int core_memory_pool_profile_allocate_count(struct core_memory_pool *self)
{
    return self->profile_allocate_calls;
}

int core_memory_pool_profile_free_count(struct core_memory_pool *self)
{
    return self->profile_free_calls;
}

void core_memory_pool_check_double_free(struct core_memory_pool *self,
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

    CORE_DEBUGGER_ASSERT(self->profile_allocate_calls >= self->profile_free_calls);
}

int core_memory_pool_profile_balance_count(struct core_memory_pool *self)
{
    return self->profile_allocate_calls - self->profile_free_calls;
}

void core_memory_pool_print_allocated_blocks(struct core_memory_pool *self)
{
        /*
    void *pointer;
    size_t size;
    */
    struct core_map_iterator iterator;

    core_map_iterator_init(&iterator, &self->allocated_blocks);

#if 0
    printf("Memory pool self= %p name= %x has %d allocated pointers\n",
                    (void *)self, self->name,
                    (int)core_map_size(&self->allocated_blocks));

    while (core_map_iterator_get_next_key_and_value(&iterator, &pointer, &size)) {
        printf("Pointer= %p Size= %zu\n", pointer, size);
    }
#endif

    core_map_iterator_destroy(&iterator);
}
