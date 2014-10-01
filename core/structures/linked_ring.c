
#include "linked_ring.h"

#include <stdlib.h>

void biosal_linked_ring_init(struct biosal_linked_ring *self, int capacity, int cell_size)
{
    biosal_ring_init(&self->ring, capacity, cell_size);
    self->next = NULL;
}

void biosal_linked_ring_destroy(struct biosal_linked_ring *self)
{
    self->next = NULL;
    biosal_ring_destroy(&self->ring);
}

struct biosal_linked_ring *biosal_linked_ring_get_next(struct biosal_linked_ring *self)
{
    return self->next;
}

void biosal_linked_ring_set_next(struct biosal_linked_ring *self, struct biosal_linked_ring *next)
{
    self->next = next;
}

struct biosal_ring *biosal_linked_ring_get_ring(struct biosal_linked_ring *self)
{
    return &self->ring;
}
