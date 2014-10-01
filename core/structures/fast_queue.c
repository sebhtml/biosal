
#include "fast_queue.h"

#include <core/system/memory.h>

#include <stdlib.h>

#define MEMORY_FAST_QUEUE 0x771872d8

void biosal_fast_queue_init(struct biosal_fast_queue *self, int bytes_per_unit)
{
    self->head = NULL;
    self->tail = NULL;
    self->recycle_bin = NULL;
    self->cell_size = bytes_per_unit;
    self->cells_per_ring = 64;
    self->size = 0;

#ifdef BIOSAL_RING_QUEUE_THREAD_SAFE
    biosal_lock_init(&self->lock);
    self->locked = BIOSAL_LOCK_UNLOCKED;
#endif
}

void biosal_fast_queue_destroy(struct biosal_fast_queue *self)
{
    struct biosal_linked_ring *next;
    self->tail = NULL;

    while (self->head != NULL) {
        next = biosal_linked_ring_get_next(self->head);
        biosal_linked_ring_destroy(self->head);
        biosal_memory_free(self->head, MEMORY_FAST_QUEUE);
        self->head = next;
    }

    while (self->recycle_bin != NULL) {
        next = biosal_linked_ring_get_next(self->recycle_bin);
        biosal_linked_ring_destroy(self->recycle_bin);
        biosal_memory_free(self->recycle_bin, MEMORY_FAST_QUEUE);
        self->recycle_bin = next;
    }

#ifdef BIOSAL_RING_QUEUE_THREAD_SAFE
    self->locked = BIOSAL_LOCK_UNLOCKED;

    biosal_lock_destroy(&self->lock);
#endif
}

int biosal_fast_queue_full(struct biosal_fast_queue *self)
{
    return BIOSAL_FALSE;
}

#ifdef BIOSAL_RING_QUEUE_THREAD_SAFE
void biosal_fast_queue_lock(struct biosal_fast_queue *self)
{
    biosal_lock_lock(&self->lock);
    self->locked = BIOSAL_LOCK_LOCKED;
}

void biosal_fast_queue_unlock(struct biosal_fast_queue *self)
{
    self->locked = BIOSAL_LOCK_UNLOCKED;
    biosal_lock_unlock(&self->lock);
}
#endif

struct biosal_linked_ring *biosal_fast_queue_get_ring(struct biosal_fast_queue *self)
{
    struct biosal_linked_ring *ring;

    if (self->recycle_bin == NULL) {
        ring = biosal_memory_allocate(sizeof(struct biosal_linked_ring), MEMORY_FAST_QUEUE);
        biosal_linked_ring_init(ring, self->cells_per_ring, self->cell_size);

        return ring;
    }

    ring = self->recycle_bin;
    self->recycle_bin = biosal_linked_ring_get_next(ring);
    biosal_linked_ring_set_next(ring, NULL);

    return ring;
}

int biosal_fast_queue_size(struct biosal_fast_queue *queue)
{
    return queue->size;
}

int biosal_fast_queue_dequeue(struct biosal_fast_queue *self, void *item)
{
    struct biosal_linked_ring *new_head;

    if (biosal_fast_queue_empty(self)) {
        return BIOSAL_FALSE;
    }

    biosal_ring_pop(biosal_linked_ring_get_ring(self->head), item);

    if (biosal_ring_is_empty(biosal_linked_ring_get_ring(self->head))
                            && self->head != self->tail) {

#ifdef BIOSAL_RING_QUEUE_THREAD_SAFE
        biosal_fast_queue_lock(self);
#endif

        new_head = biosal_linked_ring_get_next(self->head);
        biosal_linked_ring_set_next(self->head, self->recycle_bin);
        self->recycle_bin = self->head;
        self->head = new_head;

#ifdef BIOSAL_RING_QUEUE_THREAD_SAFE
        biosal_fast_queue_unlock(self);
#endif
    }

    self->size--;

    return BIOSAL_TRUE;
}

int biosal_fast_queue_empty(struct biosal_fast_queue *self)
{
    if (biosal_fast_queue_size(self) == 0) {
        return BIOSAL_TRUE;
    }
    return BIOSAL_FALSE;
}

int biosal_fast_queue_enqueue(struct biosal_fast_queue *self, void *item)
{
    biosal_fast_queue_enqueue_private(self, item);
    self->size++;
    return BIOSAL_TRUE;
}

int biosal_fast_queue_enqueue_private(struct biosal_fast_queue *self, void *item)
{
    int inserted;
    struct biosal_linked_ring *new_ring;

    if (self->tail == NULL) {

#ifdef BIOSAL_RING_QUEUE_THREAD_SAFE
        biosal_fast_queue_lock(self);
#endif

        /* the tail was assigned while this thread was waiting
         */
        if (self->tail != NULL) {

#ifdef BIOSAL_RING_QUEUE_THREAD_SAFE
            biosal_fast_queue_unlock(self);
#endif

            inserted = biosal_ring_push(biosal_linked_ring_get_ring(self->tail), item);

            if (inserted) {
                return inserted;
            }

            /* do a recursive call to add a ring
             */
            return biosal_fast_queue_enqueue_private(self, item);
        }

        self->tail = biosal_fast_queue_get_ring(self);
        self->head = self->tail;

#ifdef BIOSAL_RING_QUEUE_THREAD_SAFE
        biosal_fast_queue_unlock(self);
#endif

        inserted = biosal_ring_push(biosal_linked_ring_get_ring(self->tail), item);

        if (inserted) {
            return inserted;
        }

        return biosal_fast_queue_enqueue_private(self, item);
    }

    inserted = biosal_ring_push(biosal_linked_ring_get_ring(self->tail), item);

    /* it is full
     */
    if (!inserted) {

#ifdef BIOSAL_RING_QUEUE_THREAD_SAFE
        biosal_fast_queue_lock(self);
#endif

        /* try again
         */
        inserted = biosal_ring_push(biosal_linked_ring_get_ring(self->tail), item);

        if (inserted) {

#ifdef BIOSAL_RING_QUEUE_THREAD_SAFE
            biosal_fast_queue_unlock(self);
#endif
            return inserted;
        }

        new_ring = biosal_fast_queue_get_ring(self);
        biosal_linked_ring_set_next(self->tail, new_ring);
        self->tail = new_ring;

#ifdef BIOSAL_RING_QUEUE_THREAD_SAFE
        biosal_fast_queue_unlock(self);
#endif

        inserted = biosal_ring_push(biosal_linked_ring_get_ring(self->tail), item);
    }

    return inserted;
}


