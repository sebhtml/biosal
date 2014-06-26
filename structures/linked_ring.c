
#include "linked_ring.h"

#include <stdlib.h>

void bsal_linked_ring_init(struct bsal_linked_ring *self, int capacity, int cell_size)
{
    bsal_ring_init(&self->ring, capacity, cell_size);
    self->next = NULL;
}

void bsal_linked_ring_destroy(struct bsal_linked_ring *self)
{
    self->next = NULL;
    bsal_ring_destroy(&self->ring);
}

struct bsal_linked_ring *bsal_linked_ring_get_next(struct bsal_linked_ring *self)
{
    return self->next;
}

void bsal_linked_ring_set_next(struct bsal_linked_ring *self, struct bsal_linked_ring *next)
{
    self->next = next;
}

struct bsal_ring *bsal_linked_ring_get_ring(struct bsal_linked_ring *self)
{
    return &self->ring;
}
