
#include "fast_ring.h"

#include <core/constants.h>
#include <core/system/memory.h>
#include <core/system/atomic.h>
#include <core/system/debugger.h>

#include <performance/tracepoints/tracepoints.h>

#include <engine/thorium/message.h>

#include <string.h>
#include <stdio.h>

#include <inttypes.h>

#define MEMORY_FAST_RING 0x02d9d481

/*
 * Use memory fences for making stuff visible
 * across threads (Linux LWP in the case of Linux).
 *
 * \see http://mechanical-sympathy.blogspot.com/2011/07/memory-barriersfences.html
 */
#define USE_MEMORY_FENCE

static void *core_fast_ring_get_cell(struct core_fast_ring *self, uint64_t index);
static uint64_t core_fast_ring_increment(struct core_fast_ring *self, uint64_t index);
static int core_fast_ring_get_next_power_of_two(int value);
#ifdef CORE_FAST_RING_USE_PADDING
static uint64_t core_fast_ring_mock(struct core_fast_ring *self);
#endif

#ifdef CAS
static int core_fast_ring_push_compare_and_swap(struct core_fast_ring *self, void *element, int worker);
static int core_fast_ring_pop_and_contend(struct core_fast_ring *self, void *element);
#endif

void core_fast_ring_init(struct core_fast_ring *self, int capacity, int cell_size)
{
    self->use_multiple_producers = NO;

    /* +1 because an empty cell is needed
     */
    self->number_of_cells = core_fast_ring_get_next_power_of_two(capacity + 1);
    self->mask = self->number_of_cells - 1;

#ifdef CORE_FAST_RING_DEBUG
    printf("DEBUG RING CELLS %" PRIu64 "\n", self->number_of_cells);
#endif

    self->cell_size = cell_size;

    self->head = 0;
    self->tail = 0;

#ifdef CORE_FAST_RING_USE_CACHE
    self->head_cache = 0;
    self->tail_cache = 0;
#endif

    self->cells = core_memory_allocate(self->number_of_cells * self->cell_size, MEMORY_FAST_RING);

#ifdef CORE_FAST_RING_USE_PADDING
    /* assign values to the padding
     */
    self->consumer_padding_0 = 0;
    self->consumer_padding_1 = 0;
    self->consumer_padding_2 = 0;
    self->consumer_padding_3 = 0;
    self->consumer_padding_4 = 0;
    self->consumer_padding_5 = 0;

    self->producer_padding_0 = 0;
    self->producer_padding_1 = 0;
    self->producer_padding_2 = 0;
    self->producer_padding_3 = 0;
    self->producer_padding_4 = 0;
    self->producer_padding_5 = 0;
#endif

#ifdef CORE_RING_USE_LOCK_FOR_MULTIPLE_PRODUCERS
    core_ticket_spinlock_init(&self->lock);
#endif
}

void core_fast_ring_destroy(struct core_fast_ring *self)
{
    self->number_of_cells = 0;
    self->cell_size = 0;
    self->head = 0;
    self->tail = 0;

#ifdef CORE_FAST_RING_USE_CACHE
    self->head_cache = 0;
    self->tail_cache = 0;
#endif

    core_memory_free(self->cells, MEMORY_FAST_RING);

    self->cells = NULL;

#ifdef CORE_RING_USE_LOCK_FOR_MULTIPLE_PRODUCERS
    core_ticket_spinlock_destroy(&self->lock);
#endif
}

/*
 * Called by producer
 *
 * Can read/write tail
 * Can read/write head_cache
 * Can read head
 */
int core_fast_ring_push_from_producer(struct core_fast_ring *self, void *element)
{
    void *cell;

    CORE_DEBUGGER_ASSERT_NOT_NULL(element);
    CORE_DEBUGGER_ASSERT(self->cell_size > 0);

    if (core_fast_ring_is_full_from_producer(self)) {
        return 0;
    }

    cell = core_fast_ring_get_cell(self, self->tail);
    core_memory_copy(cell, element, self->cell_size);

    /*
     * A memory store fence is needed because we want
     * the content of the cell to be visible before
     * the tail is incremented.
     */
#ifdef USE_MEMORY_FENCE

    /*
     * The spinlock (with CORE_RING_USE_LOCK_FOR_MULTIPLE_PRODUCERS
     * and core_fast_ring_use_multiple_producers())
     * already does the job of a memory fence,
     * so doing a second fence is not a good idea.
     *
     * On x86-64, this avoids an additional mfence instruction.
     *
     * Basically, this fence is only performed if the ring is
     * a single-producer single-consumer ring (no spinlock).
     */
    if (!self->use_multiple_producers)
        CORE_MEMORY_STORE_FENCE();

#endif

    self->tail = core_fast_ring_increment(self, self->tail);

    return 1;
}

int core_fast_ring_is_full_from_producer(struct core_fast_ring *self)
{
#ifdef CORE_FAST_RING_USE_CACHE
    /* check if the head cache must be updated
     */
    if (self->head_cache == core_fast_ring_increment(self, self->tail)) {
        core_fast_ring_update_head_cache(self);
        return self->head_cache == core_fast_ring_increment(self, self->tail);
    }
    return 0;
#else
    return self->head == core_fast_ring_increment(self, self->tail);
#endif
}

int core_fast_ring_size_from_consumer(struct core_fast_ring *self)
{
    int head;
    int tail;

#ifdef CORE_FAST_RING_USE_CACHE
    /* TODO: remove me
     */
    core_fast_ring_update_tail_cache(self);
#endif

    head = self->head;

#ifdef CORE_FAST_RING_USE_CACHE
    tail = self->tail_cache;
#else
    tail = self->tail;
#endif

    if (tail < head) {
        tail += self->number_of_cells;
    }

#ifdef CORE_FAST_RING_DEBUG
    printf("from consumer tail %d head %d\n", tail, head);
#endif

    return tail - head;
}

int core_fast_ring_capacity(struct core_fast_ring *self)
{
    return self->number_of_cells - 1;
}

static uint64_t core_fast_ring_increment(struct core_fast_ring *self, uint64_t index)
{
    return  (index + 1) & self->mask;
}

static void *core_fast_ring_get_cell(struct core_fast_ring *self, uint64_t index)
{
    return ((char *)self->cells) + index * self->cell_size;
}

static int core_fast_ring_get_next_power_of_two(int value)
{
    int power_of_two;

    power_of_two = 2;

    while (power_of_two < value) {
        power_of_two *= 2;
    }

    return power_of_two;
}

/*
 * Use the padding to avoid the losing that with optimizations
 */
#ifdef CORE_FAST_RING_USE_PADDING
static uint64_t core_fast_ring_mock(struct core_fast_ring *self)
{
    uint64_t sum;

    sum = 0;

    sum += self->consumer_padding_0;
    sum += self->consumer_padding_1;
    sum += self->consumer_padding_2;
    sum += self->consumer_padding_3;
    sum += self->consumer_padding_4;
    sum += self->consumer_padding_5;

    sum += self->producer_padding_0;
    sum += self->producer_padding_1;
    sum += self->producer_padding_2;
    sum += self->producer_padding_3;
    sum += self->producer_padding_4;
    sum += self->producer_padding_5;

    return sum;
}
#endif


#ifdef CORE_FAST_RING_USE_CACHE
void core_fast_ring_update_head_cache(struct core_fast_ring *self)
{
    self->head_cache = self->head;
}
#endif

int core_fast_ring_size_from_producer(struct core_fast_ring *self)
{
    int head;
    int tail;

#ifdef CORE_FAST_RING_USE_CACHE
    /* TODO: remove me
     */
    core_fast_ring_update_head_cache(self);

    head = self->head_cache;
#else
    head = self->head;
#endif

    tail = self->tail;

    if (tail < head) {
        tail += self->number_of_cells;
    }

#ifdef CORE_FAST_RING_DEBUG
    printf("from producer tail %d head %d\n", tail, head);
#endif

    return tail - head;
}

/*
 * Called by consumer
 *
 * Can read/write head
 * Can read/write tail_cache
 * Can read tail
 */
int core_fast_ring_pop_from_consumer(struct core_fast_ring *self, void *element)
{
    void *cell;

    if (core_fast_ring_is_empty_from_consumer(self)) {
        return 0;
    }

    cell = core_fast_ring_get_cell(self, self->head);
    core_memory_copy(element, cell, self->cell_size);

    /*
     * Same here, we want the content of the cell
     * to be visible before the head is updated.
     */
#ifdef USE_MEMORY_FENCE
    CORE_MEMORY_STORE_FENCE();
#endif

    self->head = core_fast_ring_increment(self, self->head);

    return 1;
}

#ifdef CORE_FAST_RING_USE_CACHE
void core_fast_ring_update_tail_cache(struct core_fast_ring *self)
{
    self->tail_cache = self->tail;
}
#endif

int core_fast_ring_is_empty_from_consumer(struct core_fast_ring *self)
{
    CORE_DEBUGGER_ASSERT_NOT_NULL(self);

#ifdef CORE_FAST_RING_USE_CACHE
    if (self->tail_cache == self->head) {
        core_fast_ring_update_tail_cache(self);
        return self->tail_cache == self->head;
    }
#else
    return self->tail == self->head;
#endif

    return 0;
}

int core_fast_ring_is_empty_from_producer(struct core_fast_ring *self)
{
    return self->tail == self->head;
}

int core_fast_ring_empty(struct core_fast_ring *self)
{
    return core_fast_ring_size_from_producer(self) == 0;
}

#ifdef CAS
static int core_fast_ring_push_compare_and_swap(struct core_fast_ring *self, void *element, int worker)
{
    void *cell;
    int claimed_tail;
    int new_tail;
    int result;
    int steps;

    /*
     * Can't push, the ring is full.
     */
    if (core_fast_ring_is_full_from_producer(self)) {
        return 0;
    }

    tracepoint(ring, operation, "push_before_claim",
                    thorium_message_action((struct thorium_message *)element),
                    self->head, self->tail, -1, worker,
                    core_fast_ring_size_from_consumer(self),
                    core_fast_ring_capacity(self));

    /*
*/
    /*
     * Claim the cell.
     */
    result = 0;
    steps = 64;

    do {
        claimed_tail = self->tail;
        new_tail = core_fast_ring_increment(self, claimed_tail);
        result = core_atomic_compare_and_swap_int(&self->tail, claimed_tail, new_tail);
        --steps;
    } while (!result && steps);

    tracepoint(ring, operation, "push_after_claim",
                    thorium_message_action((struct thorium_message *)element),
                    self->head, self->tail, claimed_tail, worker,
                    core_fast_ring_size_from_consumer(self),
                    core_fast_ring_capacity(self));

    /*
     * 64 iterations were enough enough apparently
     */
    if (!result) {
        return 0;
    }

    /*
     * At this point, the consumer may see something before it is available
     * because the tail is already updated, but there is nothing in the cell.
     *
     * To solve that issue, any empty cell contains a marker.
     */

    /*
     * Get the cell with the tail value *before* the increment
     * and install the content in the cell.
     */
    cell = core_fast_ring_get_cell(self, claimed_tail);

    /*
     * First, copy the content, but don't copy the first 4 bytes (marker).
     */
    core_memory_copy((char *)cell + sizeof(int),
                    (char *)element + sizeof(int), self->cell_size - sizeof(int));

#ifdef USE_MEMORY_FENCE
    CORE_MEMORY_STORE_FENCE();
#endif

    /*
     * Copy first 4 bytes
     */
    core_memory_copy(cell, element, sizeof(int));

    /*
     * A memory store fence is needed because we want
     * the content of the cell to be visible.
     */
#ifdef USE_MEMORY_FENCE
    CORE_MEMORY_STORE_FENCE();
#endif

    tracepoint(ring, operation, "push_after_copy",
                    thorium_message_action((struct thorium_message *)element),
                    self->head, self->tail, claimed_tail, worker,
                    core_fast_ring_size_from_consumer(self),
                    core_fast_ring_capacity(self));

    return 1;
}

static int core_fast_ring_pop_and_contend(struct core_fast_ring *self, void *element)
{
    void *cell;
    int marker;

    /*
     * Nothing to do, it is empty.
     */
    if (core_fast_ring_is_empty_from_consumer(self)) {
        return 0;
    }

    /*
     * Do a memory fence so that the current thread sees the correct memory.
     */
    /*
#ifdef USE_MEMORY_FENCE
    CORE_MEMORY_STORE_FENCE();
#endif
*/

    tracepoint(ring, operation, "pop_before",
                    -1,
                    self->head, self->tail, -1, -1,
                    core_fast_ring_size_from_consumer(self),
                    core_fast_ring_capacity(self));

    cell = core_fast_ring_get_cell(self, self->head);

    /*
     * Check that the cell contains something useful.
     */
    core_memory_copy(&marker, cell, sizeof(marker));

    /*
     * The tail is visible before the data, we must wait in that case.
     */
    if (marker == THORIUM_MESSAGE_INVALID_ACTION) {
        return 0;
    }

    /*
     * At this point, there is something to pop, and
     * the content is also correct.
     */
    core_memory_copy(element, cell, self->cell_size);

    /*
     * The tail will sometimes be visible before
     * the data is visible. To solve that, it is required
     * to check that here.
     *
     * The only use case for core_fast_ring_push_compare_and_swap + core_fast_ring_pop_and_contend
     * is with thorium_message objects. The first member of a message is its action,
     * and an action can not be 0x00000000 (THORIUM_MESSAGE_INVALID_ACTION).
     *
     * \see http://psy-lob-saw.blogspot.com/2013/10/lock-free-mpsc-1.html
     *
     * Here we deposit a flag in the cell.
     */
    marker = THORIUM_MESSAGE_INVALID_ACTION;
    core_memory_copy(cell, &marker, sizeof(marker));

    /*
     * Same here, we want the content of the cell
     * to be visible before the head is updated.
     */

    self->head = core_fast_ring_increment(self, self->head);

#ifdef USE_MEMORY_FENCE
    CORE_MEMORY_STORE_FENCE();
#endif

    tracepoint(ring, operation, "pop_after",
                    thorium_message_action((struct thorium_message *)element),
                    self->head, self->tail, -1, -1,
                    core_fast_ring_size_from_consumer(self),
                    core_fast_ring_capacity(self));

    return 1;
}
#endif

void core_fast_ring_use_multiple_producers(struct core_fast_ring *self)
{
    int i;
    int marker;
    void *cell;

    self->use_multiple_producers = YES;

    marker = THORIUM_MESSAGE_INVALID_ACTION;

    i = 0;
    while (i < (int)self->number_of_cells) {

        cell = core_fast_ring_get_cell(self, self->head);
        core_memory_copy(cell, &marker, sizeof(marker));
        ++i;
    }
}

int core_fast_ring_push_multiple_producers(struct core_fast_ring *self, void *element, int worker)
{
#ifdef CORE_RING_USE_LOCK_FOR_MULTIPLE_PRODUCERS
    int value;

    core_ticket_spinlock_lock(&self->lock);
    value = core_fast_ring_push_from_producer(self, element);
    core_ticket_spinlock_unlock(&self->lock);

    return value;
#else
    return core_fast_ring_push_compare_and_swap(self, element, worker);
#endif
}

int core_fast_ring_pop_multiple_producers(struct core_fast_ring *self, void *element)
{
#ifdef CORE_RING_USE_LOCK_FOR_MULTIPLE_PRODUCERS
    return core_fast_ring_pop_from_consumer(self, element);
#else
    return core_fast_ring_pop_and_contend_foo(self, element);
#endif
}
