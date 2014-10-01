
#include "linked_ring.h"

#include <stdlib.h>

void core_linked_ring_init(struct core_linked_ring *self, int capacity, int cell_size)
{
    core_ring_init(&self->ring, capacity, cell_size);
    self->next = NULL;
}

void core_linked_ring_destroy(struct core_linked_ring *self)
{
    self->next = NULL;
    core_ring_destroy(&self->ring);
}

struct core_linked_ring *core_linked_ring_get_next(struct core_linked_ring *self)
{
    return self->next;
}

void core_linked_ring_set_next(struct core_linked_ring *self, struct core_linked_ring *next)
{
    self->next = next;
}

struct core_ring *core_linked_ring_get_ring(struct core_linked_ring *self)
{
    return &self->ring;
}
