
#ifndef CORE_FIFO_H
#define CORE_FIFO_H

#include "linked_ring.h"

#include <core/system/lock.h>

/*
#define CORE_RING_QUEUE_THREAD_SAFE
*/

/*
 * This is a linked list of ring that offers
 * the interface of a queue (enqueue/dequeue)
 */
struct core_fast_queue {
    struct core_linked_ring *head;
    struct core_linked_ring *tail;
    struct core_linked_ring *recycle_bin;
    int cell_size;
    int cells_per_ring;
    int size;

#ifdef CORE_RING_QUEUE_THREAD_SAFE
    struct core_lock lock;
    int locked;
#endif
};

void core_fast_queue_init(struct core_fast_queue *self, int bytes_per_unit);
void core_fast_queue_destroy(struct core_fast_queue *self);

int core_fast_queue_enqueue(struct core_fast_queue *self, void *item);
int core_fast_queue_dequeue(struct core_fast_queue *self, void *item);

int core_fast_queue_empty(struct core_fast_queue *self);
int core_fast_queue_full(struct core_fast_queue *self);
int core_fast_queue_size(struct core_fast_queue *self);

#ifdef CORE_RING_QUEUE_THREAD_SAFE
void core_fast_queue_lock(struct core_fast_queue *self);
void core_fast_queue_unlock(struct core_fast_queue *self);
#endif

struct core_linked_ring *core_fast_queue_get_ring(struct core_fast_queue *self);
int core_fast_queue_enqueue_private(struct core_fast_queue *self, void *item);

#endif
