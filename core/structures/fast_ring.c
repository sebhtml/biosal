
#include "fast_ring.h"

#include <core/system/memory.h>
#include <core/system/atomic.h>

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

void core_fast_ring_init(struct core_fast_ring *self, int capacity, int cell_size)
{
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

    if (core_fast_ring_is_full_from_producer(self)) {
        return 0;
    }

    cell = core_fast_ring_get_cell(self, self->tail);
    core_memory_copy(cell, element, self->cell_size);

#ifdef USE_MEMORY_FENCE
    core_memory_fence();
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

uint64_t core_fast_ring_increment(struct core_fast_ring *self, uint64_t index)
{
    return  (index + 1) & self->mask;
}

void *core_fast_ring_get_cell(struct core_fast_ring *self, uint64_t index)
{
    return ((char *)self->cells) + index * self->cell_size;
}

int core_fast_ring_get_next_power_of_two(int value)
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
uint64_t core_fast_ring_mock(struct core_fast_ring *self)
{
    uint64_t sum;

    sum = 0;

#ifdef CORE_FAST_RING_USE_PADDING
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
#endif

    return sum;
}

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

#ifdef USE_MEMORY_FENCE
    core_memory_fence();
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

int core_fast_ring_empty(struct core_fast_ring *self)
{
    return core_fast_ring_size_from_producer(self) == 0;
}
