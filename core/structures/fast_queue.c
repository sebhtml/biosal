
#include "fast_queue.h"

#include <core/system/memory.h>

#include <stdlib.h>

#define MEMORY_FAST_QUEUE 0x771872d8

void core_fast_queue_init(struct core_fast_queue *self, int bytes_per_unit)
{
    self->head = NULL;
    self->tail = NULL;
    self->recycle_bin = NULL;
    self->cell_size = bytes_per_unit;
    self->cells_per_ring = 64;
    self->size = 0;

#ifdef CORE_RING_QUEUE_THREAD_SAFE
    core_lock_init(&self->lock);
    self->locked = CORE_LOCK_UNLOCKED;
#endif
}

void core_fast_queue_destroy(struct core_fast_queue *self)
{
    struct core_linked_ring *next;
    self->tail = NULL;

    while (self->head != NULL) {
        next = core_linked_ring_get_next(self->head);
        core_linked_ring_destroy(self->head);
        core_memory_free(self->head, MEMORY_FAST_QUEUE);
        self->head = next;
    }

    while (self->recycle_bin != NULL) {
        next = core_linked_ring_get_next(self->recycle_bin);
        core_linked_ring_destroy(self->recycle_bin);
        core_memory_free(self->recycle_bin, MEMORY_FAST_QUEUE);
        self->recycle_bin = next;
    }

#ifdef CORE_RING_QUEUE_THREAD_SAFE
    self->locked = CORE_LOCK_UNLOCKED;

    core_lock_destroy(&self->lock);
#endif
}

int core_fast_queue_full(struct core_fast_queue *self)
{
    return CORE_FALSE;
}

#ifdef CORE_RING_QUEUE_THREAD_SAFE
void core_fast_queue_lock(struct core_fast_queue *self)
{
    core_lock_lock(&self->lock);
    self->locked = CORE_LOCK_LOCKED;
}

void core_fast_queue_unlock(struct core_fast_queue *self)
{
    self->locked = CORE_LOCK_UNLOCKED;
    core_lock_unlock(&self->lock);
}
#endif

struct core_linked_ring *core_fast_queue_get_ring(struct core_fast_queue *self)
{
    struct core_linked_ring *ring;

    if (self->recycle_bin == NULL) {
        ring = core_memory_allocate(sizeof(struct core_linked_ring), MEMORY_FAST_QUEUE);
        core_linked_ring_init(ring, self->cells_per_ring, self->cell_size);

        return ring;
    }

    ring = self->recycle_bin;
    self->recycle_bin = core_linked_ring_get_next(ring);
    core_linked_ring_set_next(ring, NULL);

    return ring;
}

int core_fast_queue_size(struct core_fast_queue *queue)
{
    return queue->size;
}

int core_fast_queue_dequeue(struct core_fast_queue *self, void *item)
{
    struct core_linked_ring *new_head;

    if (core_fast_queue_empty(self)) {
        return CORE_FALSE;
    }

    core_ring_pop(core_linked_ring_get_ring(self->head), item);

    if (core_ring_is_empty(core_linked_ring_get_ring(self->head))
                            && self->head != self->tail) {

#ifdef CORE_RING_QUEUE_THREAD_SAFE
        core_fast_queue_lock(self);
#endif

        new_head = core_linked_ring_get_next(self->head);
        core_linked_ring_set_next(self->head, self->recycle_bin);
        self->recycle_bin = self->head;
        self->head = new_head;

#ifdef CORE_RING_QUEUE_THREAD_SAFE
        core_fast_queue_unlock(self);
#endif
    }

    self->size--;

    return CORE_TRUE;
}

int core_fast_queue_empty(struct core_fast_queue *self)
{
    return !self->size;
}

int core_fast_queue_enqueue(struct core_fast_queue *self, void *item)
{
    core_fast_queue_enqueue_private(self, item);
    self->size++;
    return CORE_TRUE;
}

int core_fast_queue_enqueue_private(struct core_fast_queue *self, void *item)
{
    int inserted;
    struct core_linked_ring *new_ring;

    if (self->tail == NULL) {

#ifdef CORE_RING_QUEUE_THREAD_SAFE
        core_fast_queue_lock(self);
#endif

        /* the tail was assigned while this thread was waiting
         */
        if (self->tail != NULL) {

#ifdef CORE_RING_QUEUE_THREAD_SAFE
            core_fast_queue_unlock(self);
#endif

            inserted = core_ring_push(core_linked_ring_get_ring(self->tail), item);

            if (inserted) {
                return inserted;
            }

            /* do a recursive call to add a ring
             */
            return core_fast_queue_enqueue_private(self, item);
        }

        self->tail = core_fast_queue_get_ring(self);
        self->head = self->tail;

#ifdef CORE_RING_QUEUE_THREAD_SAFE
        core_fast_queue_unlock(self);
#endif

        inserted = core_ring_push(core_linked_ring_get_ring(self->tail), item);

        if (inserted) {
            return inserted;
        }

        return core_fast_queue_enqueue_private(self, item);
    }

    inserted = core_ring_push(core_linked_ring_get_ring(self->tail), item);

    /* it is full
     */
    if (!inserted) {

#ifdef CORE_RING_QUEUE_THREAD_SAFE
        core_fast_queue_lock(self);
#endif

        /* try again
         */
        inserted = core_ring_push(core_linked_ring_get_ring(self->tail), item);

        if (inserted) {

#ifdef CORE_RING_QUEUE_THREAD_SAFE
            core_fast_queue_unlock(self);
#endif
            return inserted;
        }

        new_ring = core_fast_queue_get_ring(self);
        core_linked_ring_set_next(self->tail, new_ring);
        self->tail = new_ring;

#ifdef CORE_RING_QUEUE_THREAD_SAFE
        core_fast_queue_unlock(self);
#endif

        inserted = core_ring_push(core_linked_ring_get_ring(self->tail), item);
    }

    return inserted;
}

