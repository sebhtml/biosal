
#ifndef CORE_LINKED_RING_H
#define CORE_LINKED_RING_H

#include "ring.h"

struct core_linked_ring;

struct core_linked_ring {
    struct core_ring ring;
    struct core_linked_ring *next;
};

void core_linked_ring_init(struct core_linked_ring *self, int capacity, int cell_size);
void core_linked_ring_destroy(struct core_linked_ring *self);
struct core_linked_ring *core_linked_ring_get_next(struct core_linked_ring *self);
void core_linked_ring_set_next(struct core_linked_ring *self, struct core_linked_ring *next);
struct core_ring *core_linked_ring_get_ring(struct core_linked_ring *self);

#endif
