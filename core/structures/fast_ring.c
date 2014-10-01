
#include "fast_ring.h"

#include <core/system/memory.h>
#include <core/system/atomic.h>

#include <string.h>
#include <stdio.h>

#include <inttypes.h>

#define MEMORY_FAST_RING 0x02d9d481

void biosal_fast_ring_init(struct biosal_fast_ring *self, int capacity, int cell_size)
{
    /* +1 because an empty cell is needed
     */
    self->number_of_cells = biosal_fast_ring_get_next_power_of_two(capacity + 1);
    self->mask = self->number_of_cells - 1;

#ifdef BIOSAL_FAST_RING_DEBUG
    printf("DEBUG RING CELLS %" PRIu64 "\n", self->number_of_cells);
#endif

    self->cell_size = cell_size;

    self->head = 0;
    self->tail = 0;

    self->head_cache = 0;
    self->tail_cache = 0;

    self->cells = biosal_memory_allocate(self->number_of_cells * self->cell_size, MEMORY_FAST_RING);

#ifdef BIOSAL_FAST_RING_USE_PADDING
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

void biosal_fast_ring_destroy(struct biosal_fast_ring *self)
{
    self->number_of_cells = 0;
    self->cell_size = 0;
    self->head = 0;
    self->tail = 0;
    self->head_cache = 0;
    self->tail_cache = 0;

    biosal_memory_free(self->cells, MEMORY_FAST_RING);

    self->cells = NULL;
}

/*
 * Called by producer
 *
 * Can read/write tail
 * Can read/write head_cache
 * Can read head
 */
int biosal_fast_ring_push_from_producer(struct biosal_fast_ring *self, void *element)
{
    void *cell;

    if (biosal_fast_ring_is_full_from_producer(self)) {
        return 0;
    }

    cell = biosal_fast_ring_get_cell(self, self->tail);
    biosal_memory_copy(cell, element, self->cell_size);

    biosal_memory_fence();

    self->tail = biosal_fast_ring_increment(self, self->tail);

    return 1;
}

int biosal_fast_ring_is_full_from_producer(struct biosal_fast_ring *self)
{
    /* check if the head cache must be updated
     */
    if (self->head_cache == biosal_fast_ring_increment(self, self->tail)) {
        biosal_fast_ring_update_head_cache(self);
        return self->head_cache == biosal_fast_ring_increment(self, self->tail);
    }
    return 0;
}

int biosal_fast_ring_size_from_consumer(struct biosal_fast_ring *self)
{
    int head;
    int tail;

    /* TODO: remove me
     */
    biosal_fast_ring_update_tail_cache(self);

    head = self->head;
    tail = self->tail_cache;

    if (tail < head) {
        tail += self->number_of_cells;
    }

#ifdef BIOSAL_FAST_RING_DEBUG
    printf("from consumer tail %d head %d\n", tail, head);
#endif

    return tail - head;
}

int biosal_fast_ring_capacity(struct biosal_fast_ring *self)
{
    return self->number_of_cells - 1;
}

uint64_t biosal_fast_ring_increment(struct biosal_fast_ring *self, uint64_t index)
{
    return  (index + 1) & self->mask;
}

void *biosal_fast_ring_get_cell(struct biosal_fast_ring *self, uint64_t index)
{
    return ((char *)self->cells) + index * self->cell_size;
}

int biosal_fast_ring_get_next_power_of_two(int value)
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
uint64_t biosal_fast_ring_mock(struct biosal_fast_ring *self)
{
    uint64_t sum;

    sum = 0;

#ifdef BIOSAL_FAST_RING_USE_PADDING
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

void biosal_fast_ring_update_head_cache(struct biosal_fast_ring *self)
{
    self->head_cache = self->head;
}

int biosal_fast_ring_size_from_producer(struct biosal_fast_ring *self)
{
    int head;
    int tail;

    /* TODO: remove me
     */
    biosal_fast_ring_update_head_cache(self);

    head = self->head_cache;
    tail = self->tail;

    if (tail < head) {
        tail += self->number_of_cells;
    }

#ifdef BIOSAL_FAST_RING_DEBUG
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
int biosal_fast_ring_pop_from_consumer(struct biosal_fast_ring *self, void *element)
{
    void *cell;

    if (biosal_fast_ring_is_empty_from_consumer(self)) {
        return 0;
    }

    cell = biosal_fast_ring_get_cell(self, self->head);
    biosal_memory_copy(element, cell, self->cell_size);

    biosal_memory_fence();

    self->head = biosal_fast_ring_increment(self, self->head);

    return 1;
}

void biosal_fast_ring_update_tail_cache(struct biosal_fast_ring *self)
{
    self->tail_cache = self->tail;
}

int biosal_fast_ring_is_empty_from_consumer(struct biosal_fast_ring *self)
{
    if (self->tail_cache == self->head) {
        biosal_fast_ring_update_tail_cache(self);
        return self->tail_cache == self->head;
    }

    return 0;
}

int biosal_fast_ring_empty(struct biosal_fast_ring *self)
{
    return biosal_fast_ring_size_from_producer(self) == 0;
}
