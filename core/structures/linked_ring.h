
#ifndef BSAL_LINKED_RING_H
#define BSAL_LINKED_RING_H

#include "ring.h"

struct bsal_linked_ring;

struct bsal_linked_ring {
    struct bsal_ring ring;
    struct bsal_linked_ring *next;
};

void bsal_linked_ring_init(struct bsal_linked_ring *self, int capacity, int cell_size);
void bsal_linked_ring_destroy(struct bsal_linked_ring *self);
struct bsal_linked_ring *bsal_linked_ring_get_next(struct bsal_linked_ring *self);
void bsal_linked_ring_set_next(struct bsal_linked_ring *self, struct bsal_linked_ring *next);
struct bsal_ring *bsal_linked_ring_get_ring(struct bsal_linked_ring *self);

#endif
