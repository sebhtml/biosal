
#ifndef CORE_FIFO_H
#define CORE_FIFO_H

#include "linked_ring.h"

#include <core/system/spinlock.h>

/*
#define CORE_RING_QUEUE_THREAD_SAFE
*/

struct core_memory_pool;

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
    struct core_spinlock lock;
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

int core_fast_queue_capacity(struct core_fast_queue *self);
void core_fast_queue_set_memory_pool(struct core_fast_queue *self,
                struct core_memory_pool *pool);

#endif
