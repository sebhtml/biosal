
#include "fast_queue.h"

#include <core/system/memory.h>

#include <stdlib.h>

#define MEMORY_FAST_QUEUE 0x771872d8

void bsal_fast_queue_init(struct bsal_fast_queue *self, int bytes_per_unit)
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

void bsal_fast_queue_destroy(struct bsal_fast_queue *self)
{
    struct bsal_linked_ring *next;
    self->tail = NULL;

    while (self->head != NULL) {
        next = bsal_linked_ring_get_next(self->head);
        bsal_linked_ring_destroy(self->head);
        bsal_memory_free(self->head, MEMORY_FAST_QUEUE);
        self->head = next;
    }

    while (self->recycle_bin != NULL) {
        next = bsal_linked_ring_get_next(self->recycle_bin);
        bsal_linked_ring_destroy(self->recycle_bin);
        bsal_memory_free(self->recycle_bin, MEMORY_FAST_QUEUE);
        self->recycle_bin = next;
    }

#ifdef BSAL_RING_QUEUE_THREAD_SAFE
    self->locked = BSAL_LOCK_UNLOCKED;

    bsal_lock_destroy(&self->lock);
#endif
}

int bsal_fast_queue_full(struct bsal_fast_queue *self)
{
    return BSAL_FALSE;
}

#ifdef BSAL_RING_QUEUE_THREAD_SAFE
void bsal_fast_queue_lock(struct bsal_fast_queue *self)
{
    bsal_lock_lock(&self->lock);
    self->locked = BSAL_LOCK_LOCKED;
}

void bsal_fast_queue_unlock(struct bsal_fast_queue *self)
{
    self->locked = BSAL_LOCK_UNLOCKED;
    bsal_lock_unlock(&self->lock);
}
#endif

struct bsal_linked_ring *bsal_fast_queue_get_ring(struct bsal_fast_queue *self)
{
    struct bsal_linked_ring *ring;

    if (self->recycle_bin == NULL) {
        ring = bsal_memory_allocate(sizeof(struct bsal_linked_ring), MEMORY_FAST_QUEUE);
        bsal_linked_ring_init(ring, self->cells_per_ring, self->cell_size);

        return ring;
    }

    ring = self->recycle_bin;
    self->recycle_bin = bsal_linked_ring_get_next(ring);
    bsal_linked_ring_set_next(ring, NULL);

    return ring;
}

int bsal_fast_queue_size(struct bsal_fast_queue *queue)
{
    return queue->size;
}

int bsal_fast_queue_dequeue(struct bsal_fast_queue *self, void *item)
{
    struct bsal_linked_ring *new_head;

    if (bsal_fast_queue_empty(self)) {
        return BSAL_FALSE;
    }

    bsal_ring_pop(bsal_linked_ring_get_ring(self->head), item);

    if (bsal_ring_is_empty(bsal_linked_ring_get_ring(self->head))
                            && self->head != self->tail) {

#ifdef BSAL_RING_QUEUE_THREAD_SAFE
        bsal_fast_queue_lock(self);
#endif

        new_head = bsal_linked_ring_get_next(self->head);
        bsal_linked_ring_set_next(self->head, self->recycle_bin);
        self->recycle_bin = self->head;
        self->head = new_head;

#ifdef BSAL_RING_QUEUE_THREAD_SAFE
        bsal_fast_queue_unlock(self);
#endif
    }

    self->size--;

    return BSAL_TRUE;
}

int bsal_fast_queue_empty(struct bsal_fast_queue *self)
{
    if (bsal_fast_queue_size(self) == 0) {
        return BSAL_TRUE;
    }
    return BSAL_FALSE;
}

int bsal_fast_queue_enqueue(struct bsal_fast_queue *self, void *item)
{
    bsal_fast_queue_enqueue_private(self, item);
    self->size++;
    return BSAL_TRUE;
}

int bsal_fast_queue_enqueue_private(struct bsal_fast_queue *self, void *item)
{
    int inserted;
    struct bsal_linked_ring *new_ring;

    if (self->tail == NULL) {

#ifdef BSAL_RING_QUEUE_THREAD_SAFE
        bsal_fast_queue_lock(self);
#endif

        /* the tail was assigned while this thread was waiting
         */
        if (self->tail != NULL) {

#ifdef BSAL_RING_QUEUE_THREAD_SAFE
            bsal_fast_queue_unlock(self);
#endif

            inserted = bsal_ring_push(bsal_linked_ring_get_ring(self->tail), item);

            if (inserted) {
                return inserted;
            }

            /* do a recursive call to add a ring
             */
            return bsal_fast_queue_enqueue_private(self, item);
        }

        self->tail = bsal_fast_queue_get_ring(self);
        self->head = self->tail;

#ifdef BSAL_RING_QUEUE_THREAD_SAFE
        bsal_fast_queue_unlock(self);
#endif

        inserted = bsal_ring_push(bsal_linked_ring_get_ring(self->tail), item);

        if (inserted) {
            return inserted;
        }

        return bsal_fast_queue_enqueue_private(self, item);
    }

    inserted = bsal_ring_push(bsal_linked_ring_get_ring(self->tail), item);

    /* it is full
     */
    if (!inserted) {

#ifdef BSAL_RING_QUEUE_THREAD_SAFE
        bsal_fast_queue_lock(self);
#endif

        /* try again
         */
        inserted = bsal_ring_push(bsal_linked_ring_get_ring(self->tail), item);

        if (inserted) {

#ifdef BSAL_RING_QUEUE_THREAD_SAFE
            bsal_fast_queue_unlock(self);
#endif
            return inserted;
        }

        new_ring = bsal_fast_queue_get_ring(self);
        bsal_linked_ring_set_next(self->tail, new_ring);
        self->tail = new_ring;

#ifdef BSAL_RING_QUEUE_THREAD_SAFE
        bsal_fast_queue_unlock(self);
#endif

        inserted = bsal_ring_push(bsal_linked_ring_get_ring(self->tail), item);
    }

    return inserted;
}


