
#include "ring.h"

#include <core/system/memory.h>

#include <string.h>

#define MEMORY_RING 0x28fc4a42

void core_ring_init(struct core_ring *self, int capacity, int cell_size)
{
    self->number_of_cells = capacity + 1;
    self->cell_size = cell_size;
    self->head = 0;
    self->tail = 0;

    self->cells = core_memory_allocate(self->number_of_cells * self->cell_size, MEMORY_RING);
}

void core_ring_destroy(struct core_ring *self)
{
    self->number_of_cells = 0;
    self->cell_size = 0;
    self->head = 0;
    self->tail = 0;

    core_memory_free(self->cells, MEMORY_RING);

    self->cells = NULL;
}

int core_ring_push(struct core_ring *self, void *element)
{
    void *cell;

    if (core_ring_is_full(self)) {
        return CORE_FALSE;
    }

    cell = core_ring_get_cell(self, self->tail);
    core_memory_copy(cell, element, self->cell_size);
    self->tail = core_ring_increment(self, self->tail);

    return CORE_TRUE;
}

int core_ring_pop(struct core_ring *self, void *element)
{
    void *cell;

    if (core_ring_is_empty(self)) {
        return CORE_FALSE;
    }

    cell = core_ring_get_cell(self, self->head);
    core_memory_copy(element, cell, self->cell_size);
    self->head = core_ring_increment(self, self->head);

    return CORE_TRUE;
}

int core_ring_is_full(struct core_ring *self)
{
    return core_ring_increment(self, self->tail) == self->head;
}

int core_ring_is_empty(struct core_ring *self)
{
    return self->head == self->tail;
}

int core_ring_size(struct core_ring *self)
{
    int tail;

    tail = self->tail;

    if (tail < self->head) {
        tail += self->number_of_cells;
    }

    return tail - self->head;
}

int core_ring_capacity(struct core_ring *self)
{
    return self->number_of_cells - 1;
}

int core_ring_increment(struct core_ring *self, int index)
{
    return  (index + 1) % self->number_of_cells;
}

void *core_ring_get_cell(struct core_ring *self, int index)
{
    return ((char *)self->cells) + index * self->cell_size;
}
