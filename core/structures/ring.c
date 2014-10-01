
#include "ring.h"

#include <core/system/memory.h>

#include <string.h>

#define MEMORY_RING 0x28fc4a42

void biosal_ring_init(struct biosal_ring *self, int capacity, int cell_size)
{
    self->number_of_cells = capacity + 1;
    self->cell_size = cell_size;
    self->head = 0;
    self->tail = 0;

    self->cells = biosal_memory_allocate(self->number_of_cells * self->cell_size, MEMORY_RING);
}

void biosal_ring_destroy(struct biosal_ring *self)
{
    self->number_of_cells = 0;
    self->cell_size = 0;
    self->head = 0;
    self->tail = 0;

    biosal_memory_free(self->cells, MEMORY_RING);

    self->cells = NULL;
}

int biosal_ring_push(struct biosal_ring *self, void *element)
{
    void *cell;

    if (biosal_ring_is_full(self)) {
        return BIOSAL_FALSE;
    }

    cell = biosal_ring_get_cell(self, self->tail);
    biosal_memory_copy(cell, element, self->cell_size);
    self->tail = biosal_ring_increment(self, self->tail);

    return BIOSAL_TRUE;
}

int biosal_ring_pop(struct biosal_ring *self, void *element)
{
    void *cell;

    if (biosal_ring_is_empty(self)) {
        return BIOSAL_FALSE;
    }

    cell = biosal_ring_get_cell(self, self->head);
    biosal_memory_copy(element, cell, self->cell_size);
    self->head = biosal_ring_increment(self, self->head);

    return BIOSAL_TRUE;
}

int biosal_ring_is_full(struct biosal_ring *self)
{
    return biosal_ring_increment(self, self->tail) == self->head;
}

int biosal_ring_is_empty(struct biosal_ring *self)
{
    return self->head == self->tail;
}

int biosal_ring_size(struct biosal_ring *self)
{
    int tail;

    tail = self->tail;

    if (tail < self->head) {
        tail += self->number_of_cells;
    }

    return tail - self->head;
}

int biosal_ring_capacity(struct biosal_ring *self)
{
    return self->number_of_cells - 1;
}

int biosal_ring_increment(struct biosal_ring *self, int index)
{
    return  (index + 1) % self->number_of_cells;
}

void *biosal_ring_get_cell(struct biosal_ring *self, int index)
{
    return ((char *)self->cells) + index * self->cell_size;
}
