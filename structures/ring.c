
#include "ring.h"

#include <system/memory.h>

#include <string.h>

void bsal_ring_init(struct bsal_ring *self, int capacity, int cell_size)
{
    self->number_of_cells = capacity + 1;
    self->cell_size = cell_size;
    self->head = 0;
    self->tail = 0;

    self->cells = bsal_memory_allocate(self->number_of_cells * self->cell_size);
}

void bsal_ring_destroy(struct bsal_ring *self)
{
    self->number_of_cells = 0;
    self->cell_size = 0;
    self->head = 0;
    self->tail = 0;

    bsal_memory_free(self->cells);

    self->cells = NULL;
}

int bsal_ring_push(struct bsal_ring *self, void *element)
{
    void *cell;

    if (bsal_ring_is_full(self)) {
        return BSAL_FALSE;
    }

    cell = bsal_ring_get_cell(self, self->tail);
    memcpy(cell, element, self->cell_size);
    self->tail = bsal_ring_increment(self, self->tail);

    return BSAL_TRUE;
}

int bsal_ring_pop(struct bsal_ring *self, void *element)
{
    void *cell;

    if (bsal_ring_is_empty(self)) {
        return BSAL_FALSE;
    }

    cell = bsal_ring_get_cell(self, self->head);
    memcpy(element, cell, self->cell_size);
    self->head = bsal_ring_increment(self, self->head);

    return BSAL_TRUE;
}

int bsal_ring_is_full(struct bsal_ring *self)
{
    return bsal_ring_increment(self, self->tail) == self->head;
}

int bsal_ring_is_empty(struct bsal_ring *self)
{
    return self->head == self->tail;
}

int bsal_ring_size(struct bsal_ring *self)
{
    int tail;

    tail = self->tail;

    if (tail < self->head) {
        tail += self->number_of_cells;
    }

    return tail - self->head;
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
