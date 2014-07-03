
#include "ring_queue.h"

#include <core/system/memory.h>

#include <stdlib.h>

void bsal_ring_queue_init(struct bsal_ring_queue *self, int bytes_per_unit)
{
    self->head = NULL;
    self->tail = NULL;
    self->recycle_bin = NULL;
    self->cell_size = bytes_per_unit;
    self->cells_per_ring = 64;
    self->size = 0;

#ifdef BSAL_RING_QUEUE_THREAD_SAFE
    bsal_lock_init(&self->lock);
    self->locked = BSAL_LOCK_UNLOCKED;
#endif
}

void bsal_ring_queue_destroy(struct bsal_ring_queue *self)
{
    struct bsal_linked_ring *next;
    self->tail = NULL;

    while (self->head != NULL) {
        next = bsal_linked_ring_get_next(self->head);
        bsal_linked_ring_destroy(self->head);
        self->head = next;
    }

    while (self->recycle_bin != NULL) {
        next = bsal_linked_ring_get_next(self->recycle_bin);
        bsal_linked_ring_destroy(self->recycle_bin);
        self->recycle_bin = next;
    }

#ifdef BSAL_RING_QUEUE_THREAD_SAFE
    self->locked = BSAL_LOCK_UNLOCKED;

    bsal_lock_destroy(&self->lock);
#endif
}

int bsal_ring_queue_full(struct bsal_ring_queue *self)
{
    return BSAL_FALSE;
}

#ifdef BSAL_RING_QUEUE_THREAD_SAFE
void bsal_ring_queue_lock(struct bsal_ring_queue *self)
{
    bsal_lock_lock(&self->lock);
    self->locked = BSAL_LOCK_LOCKED;
}

void bsal_ring_queue_unlock(struct bsal_ring_queue *self)
{
    self->locked = BSAL_LOCK_UNLOCKED;
    bsal_lock_unlock(&self->lock);
}
#endif

struct bsal_linked_ring *bsal_ring_queue_get_ring(struct bsal_ring_queue *self)
{
    struct bsal_linked_ring *ring;

    if (self->recycle_bin == NULL) {
        ring = bsal_memory_allocate(sizeof(struct bsal_linked_ring));
        bsal_linked_ring_init(ring, self->cells_per_ring, self->cell_size);

        return ring;
    }

    ring = self->recycle_bin;
    self->recycle_bin = bsal_linked_ring_get_next(self->recycle_bin);

    return ring;
}


