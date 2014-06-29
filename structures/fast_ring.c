
#include "fast_ring.h"

#include <system/memory.h>
#include <system/atomic.h>

#include <string.h>
#include <stdio.h>

#include <inttypes.h>

void bsal_fast_ring_init(struct bsal_fast_ring *self, int capacity, int cell_size)
{
    /* +1 because an empty cell is needed
     */
    self->number_of_cells = bsal_fast_ring_get_next_power_of_two(capacity + 1);
    self->mask = self->number_of_cells - 1;

#ifdef BSAL_FAST_RING_DEBUG
    printf("DEBUG RING CELLS %" PRIu64 "\n", self->number_of_cells);
#endif

    self->cell_size = cell_size;
    self->head.value = 0;
    self->tail.value = 0;
    self->head_cache.value = 0;
    self->tail_cache.value = 0;

    self->cells = bsal_memory_allocate(self->number_of_cells * self->cell_size);
}

void bsal_fast_ring_destroy(struct bsal_fast_ring *self)
{
    self->number_of_cells = 0;
    self->cell_size = 0;
    self->head.value = 0;
    self->tail.value = 0;
    self->head_cache.value = 0;
    self->tail_cache.value = 0;

    bsal_memory_free(self->cells);

    self->cells = NULL;
}

/*
 * Called by producer
 *
 * Can read/write tail
 * Can read/write head_cache
 * Can read head
 */
int bsal_fast_ring_push_from_producer(struct bsal_fast_ring *self, void *element)
{
    void *cell;

    if (bsal_fast_ring_is_full_from_producer(self)) {
        return 0;
    }

    cell = bsal_fast_ring_get_cell(self, self->tail.value);
    memcpy(cell, element, self->cell_size);
    self->tail.value = bsal_fast_ring_increment(self, self->tail.value);

    return 1;
}

int bsal_fast_ring_is_full_from_producer(struct bsal_fast_ring *self)
{
    /* check if the head cache must be updated
     */
    if (self->head_cache.value == bsal_fast_ring_increment(self, self->tail.value)) {
        bsal_fast_ring_update_head_cache(self);
        return self->head_cache.value == bsal_fast_ring_increment(self, self->tail.value);
    }
    return 0;
}

/*
 * Called by consumer
 *
 * Can read/write head
 * Can read/write tail_cache
 * Can read tail
 */
int bsal_fast_ring_pop_from_consumer(struct bsal_fast_ring *self, void *element)
{
    void *cell;

    if (bsal_fast_ring_is_empty_from_consumer(self)) {
        return 0;
    }

    cell = bsal_fast_ring_get_cell(self, self->head.value);
    memcpy(element, cell, self->cell_size);
    self->head.value = bsal_fast_ring_increment(self, self->head.value);

    return 1;
}

int bsal_fast_ring_is_empty_from_consumer(struct bsal_fast_ring *self)
{
    if (self->tail_cache.value == self->head.value) {
        bsal_fast_ring_update_tail_cache(self);
        return self->tail_cache.value == self->head.value;
    }

    return 0;
}

int bsal_fast_ring_size_from_consumer(struct bsal_fast_ring *self)
{
    int head;
    int tail;

    /* TODO: remove me
     */
    bsal_fast_ring_update_tail_cache(self);

    head = self->head.value;
    tail = self->tail_cache.value;

    if (tail < head) {
        tail += self->number_of_cells;
    }

#ifdef BSAL_FAST_RING_DEBUG
    printf("from consumer tail %d head %d\n", tail, head);
#endif

    return tail - head;
}

int bsal_fast_ring_size_from_producer(struct bsal_fast_ring *self)
{
    int head;
    int tail;

    /* TODO: remove me
     */
    bsal_fast_ring_update_head_cache(self);

    head = self->head_cache.value;
    tail = self->tail.value;

    if (tail < head) {
        tail += self->number_of_cells;
    }

#ifdef BSAL_FAST_RING_DEBUG
    printf("from producer tail %d head %d\n", tail, head);
#endif

    return tail - head;
}

int bsal_fast_ring_capacity(struct bsal_fast_ring *self)
{
    return self->number_of_cells - 1;
}

uint64_t bsal_fast_ring_increment(struct bsal_fast_ring *self, uint64_t index)
{
    return  (index + 1) & self->mask;
}

void *bsal_fast_ring_get_cell(struct bsal_fast_ring *self, uint64_t index)
{
    return ((char *)self->cells) + index * self->cell_size;
}

int bsal_fast_ring_get_next_power_of_two(int value)
{
    int power_of_two;

    power_of_two = 2;

    while (power_of_two < value) {
        power_of_two *= 2;
    }

    return power_of_two;
}

void bsal_fast_ring_update_head_cache(struct bsal_fast_ring *self)
{
    self->head_cache.value = self->head.value;
}

void bsal_fast_ring_update_tail_cache(struct bsal_fast_ring *self)
{
    self->tail_cache.value = self->tail.value;
}
