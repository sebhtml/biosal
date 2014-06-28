
#include "ring.h"

#include <system/memory.h>
#include <system/atomic.h>

#include <string.h>

void bsal_ring_init(struct bsal_ring *self, int capacity, int cell_size)
{
    self->number_of_cells = capacity + 1;
    self->cell_size = cell_size;
    self->head = 0;
    self->tail = 0;

    self->cells = bsal_allocate(self->number_of_cells * self->cell_size);
    self->atomic = 0;
}

void bsal_ring_destroy(struct bsal_ring *self)
{
    self->number_of_cells = 0;
    self->cell_size = 0;
    self->head = 0;
    self->tail = 0;

    bsal_free(self->cells);

    self->cells = NULL;
}

int bsal_ring_push(struct bsal_ring *self, void *element)
{
    void *cell;

    if (bsal_ring_is_full(self)) {
        return 0;
    }

    cell = bsal_ring_get_cell(self, bsal_ring_get_tail(self));
    memcpy(cell, element, self->cell_size);
    bsal_ring_increment_tail(self);

    return 1;
}

int bsal_ring_pop(struct bsal_ring *self, void *element)
{
    void *cell;

    if (bsal_ring_is_empty(self)) {
        return 0;
    }

    cell = bsal_ring_get_cell(self, bsal_ring_get_head(self));
    memcpy(element, cell, self->cell_size);
    bsal_ring_increment_head(self);

    return 1;
}

int bsal_ring_is_full(struct bsal_ring *self)
{
    return bsal_ring_increment(self, bsal_ring_get_tail(self)) == bsal_ring_get_head(self);
}

int bsal_ring_is_empty(struct bsal_ring *self)
{
    return bsal_ring_get_tail(self) == bsal_ring_get_head(self);
}

int bsal_ring_size(struct bsal_ring *self)
{
    int tail;
    int head;

    head = bsal_ring_get_head(self);
    tail = bsal_ring_get_tail(self);

    if (tail < head) {
        tail += self->number_of_cells;
    }

    return tail - head;
}

int bsal_ring_capacity(struct bsal_ring *self)
{
    return self->number_of_cells - 1;
}

int bsal_ring_increment(struct bsal_ring *self, int index)
{
    return  (index + 1) % self->number_of_cells;
}

void *bsal_ring_get_cell(struct bsal_ring *self, int index)
{
    return ((char *)self->cells) + index * self->cell_size;
}

void bsal_ring_enable_atomicity(struct bsal_ring *self)
{
    self->atomic = 1;
}

void bsal_ring_disable_atomicity(struct bsal_ring *self)
{
    self->atomic = 0;
}

int bsal_ring_get_head(struct bsal_ring *self)
{
    if (self->atomic) {
        return bsal_atomic_read_int(&self->head);
    }

    return self->head;
}

int bsal_ring_get_tail(struct bsal_ring *self)
{
    if (self->atomic) {
        return bsal_atomic_read_int(&self->tail);
    }

    return self->tail;
}

void bsal_ring_increment_head(struct bsal_ring *self)
{
    int new_value;

    new_value = bsal_ring_increment(self, self->head);

    if (self->atomic) {
        bsal_atomic_compare_and_swap_int(&self->head, self->head, new_value);
    } else {
        self->head = new_value;
    }
}

void bsal_ring_increment_tail(struct bsal_ring *self)
{
    int new_value;

    new_value = bsal_ring_increment(self, self->tail);

    if (self->atomic) {
        bsal_atomic_compare_and_swap_int(&self->tail, self->tail, new_value);
    } else {
        self->tail = new_value;
    }
}
