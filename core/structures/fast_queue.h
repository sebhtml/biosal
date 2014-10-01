
#ifndef BIOSAL_FIFO_H
#define BIOSAL_FIFO_H

#include "linked_ring.h"

#include <core/system/lock.h>

/*
#define BIOSAL_RING_QUEUE_THREAD_SAFE
*/

/*
 * This is a linked list of ring that offers
 * the interface of a queue (enqueue/dequeue)
 */
struct biosal_fast_queue {
    struct biosal_linked_ring *head;
    struct biosal_linked_ring *tail;
    struct biosal_linked_ring *recycle_bin;
    int cell_size;
    int cells_per_ring;
    int size;

#ifdef BIOSAL_RING_QUEUE_THREAD_SAFE
    struct biosal_lock lock;
    int locked;
#endif
};

void biosal_fast_queue_init(struct biosal_fast_queue *self, int bytes_per_unit);
void biosal_fast_queue_destroy(struct biosal_fast_queue *self);

int biosal_fast_queue_enqueue(struct biosal_fast_queue *self, void *item);
int biosal_fast_queue_dequeue(struct biosal_fast_queue *self, void *item);

int biosal_fast_queue_empty(struct biosal_fast_queue *self);
int biosal_fast_queue_full(struct biosal_fast_queue *self);
int biosal_fast_queue_size(struct biosal_fast_queue *self);

#ifdef BIOSAL_RING_QUEUE_THREAD_SAFE
void biosal_fast_queue_lock(struct biosal_fast_queue *self);
void biosal_fast_queue_unlock(struct biosal_fast_queue *self);
#endif

struct biosal_linked_ring *biosal_fast_queue_get_ring(struct biosal_fast_queue *self);
int biosal_fast_queue_enqueue_private(struct biosal_fast_queue *self, void *item);

#endif
