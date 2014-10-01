
#ifndef BIOSAL_LINKED_RING_H
#define BIOSAL_LINKED_RING_H

#include "ring.h"

struct biosal_linked_ring;

struct biosal_linked_ring {
    struct biosal_ring ring;
    struct biosal_linked_ring *next;
};

void biosal_linked_ring_init(struct biosal_linked_ring *self, int capacity, int cell_size);
void biosal_linked_ring_destroy(struct biosal_linked_ring *self);
struct biosal_linked_ring *biosal_linked_ring_get_next(struct biosal_linked_ring *self);
void biosal_linked_ring_set_next(struct biosal_linked_ring *self, struct biosal_linked_ring *next);
struct biosal_ring *biosal_linked_ring_get_ring(struct biosal_linked_ring *self);

#endif
