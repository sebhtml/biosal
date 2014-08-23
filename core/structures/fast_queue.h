
#ifndef BSAL_FIFO_H
#define BSAL_FIFO_H

#include "linked_ring.h"

#include <core/system/lock.h>

/*
#define BSAL_RING_QUEUE_THREAD_SAFE
*/

/*
 * This is a linked list of ring that offers
 * the interface of a queue (enqueue/dequeue)
 */
struct bsal_fast_queue {
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

void bsal_fast_queue_init(struct bsal_fast_queue *self, int bytes_per_unit);
void bsal_fast_queue_destroy(struct bsal_fast_queue *self);

int bsal_fast_queue_enqueue(struct bsal_fast_queue *self, void *item);
int bsal_fast_queue_dequeue(struct bsal_fast_queue *self, void *item);

int bsal_fast_queue_empty(struct bsal_fast_queue *self);
int bsal_fast_queue_full(struct bsal_fast_queue *self);
int bsal_fast_queue_size(struct bsal_fast_queue *self);

#ifdef BSAL_RING_QUEUE_THREAD_SAFE
void bsal_fast_queue_lock(struct bsal_fast_queue *self);
void bsal_fast_queue_unlock(struct bsal_fast_queue *self);
#endif

struct bsal_linked_ring *bsal_fast_queue_get_ring(struct bsal_fast_queue *self);
int bsal_fast_queue_enqueue_private(struct bsal_fast_queue *self, void *item);

#endif
