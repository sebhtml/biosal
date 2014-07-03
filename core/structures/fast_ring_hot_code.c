
#include "fast_ring.h"

#include <string.h>
#include <stdio.h>

void bsal_fast_ring_update_head_cache(struct bsal_fast_ring *self)
{
    self->head_cache = self->head;
}

int bsal_fast_ring_size_from_producer(struct bsal_fast_ring *self)
{
    int head;
    int tail;

    /* TODO: remove me
     */
    bsal_fast_ring_update_head_cache(self);

    head = self->head_cache;
    tail = self->tail;

    if (tail < head) {
        tail += self->number_of_cells;
    }

#ifdef BSAL_FAST_RING_DEBUG
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
int bsal_fast_ring_pop_from_consumer(struct bsal_fast_ring *self, void *element)
{
    void *cell;

    if (bsal_fast_ring_is_empty_from_consumer(self)) {
        return 0;
    }

    cell = bsal_fast_ring_get_cell(self, self->head);
    memcpy(element, cell, self->cell_size);
    self->head = bsal_fast_ring_increment(self, self->head);

    return 1;
}

void bsal_fast_ring_update_tail_cache(struct bsal_fast_ring *self)
{
    self->tail_cache = self->tail;
}

int bsal_fast_ring_is_empty_from_consumer(struct bsal_fast_ring *self)
{
    if (self->tail_cache == self->head) {
        bsal_fast_ring_update_tail_cache(self);
        return self->tail_cache == self->head;
    }

    return 0;
}


