
#ifndef BSAL_FIFO_H
#define BSAL_FIFO_H

#include "linked_ring.h"

#include <core/system/lock.h>

/*
#define BSAL_RING_QUEUE_THREAD_SAFE
*/

struct bsal_ring_queue {
    struct bsal_linked_ring *head;
    struct bsal_linked_ring *tail;
    struct bsal_linked_ring *recycle_bin;
    int cell_size;
    int cells_per_ring;
    int size;

#ifdef BSAL_RING_QUEUE_THREAD_SAFE
    struct bsal_lock lock;
    int locked;
#endif
};

void bsal_ring_queue_init(struct bsal_ring_queue *self, int bytes_per_unit);
void bsal_ring_queue_destroy(struct bsal_ring_queue *self);

int bsal_ring_queue_enqueue(struct bsal_ring_queue *self, void *item);
int bsal_ring_queue_dequeue(struct bsal_ring_queue *self, void *item);

int bsal_ring_queue_empty(struct bsal_ring_queue *self);
int bsal_ring_queue_full(struct bsal_ring_queue *self);
int bsal_ring_queue_size(struct bsal_ring_queue *queue);

#ifdef BSAL_RING_QUEUE_THREAD_SAFE
void bsal_ring_queue_lock(struct bsal_ring_queue *self);
void bsal_ring_queue_unlock(struct bsal_ring_queue *self);
#endif

struct bsal_linked_ring *bsal_ring_queue_get_ring(struct bsal_ring_queue *self);
int bsal_ring_queue_enqueue_private(struct bsal_ring_queue *self, void *item);

#endif
