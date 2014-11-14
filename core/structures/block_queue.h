
#ifndef CORE_BLOCK_QUEUE_H
#define CORE_BLOCK_QUEUE_H

#include "vector.h"

struct core_memory_pool;

/*
 * A FIFO (first in, first out)
 */
struct core_block_queue {
    struct core_vector vector;
    int enqueue_index;
    int dequeue_index;
    int size;
    int bytes_per_element;
};

void core_block_queue_init(struct core_block_queue *self, int bytes_per_unit);
void core_block_queue_destroy(struct core_block_queue *self);

/*
 * \returns 1 if successful, 0 otherwise
 */
int core_block_queue_enqueue(struct core_block_queue *self, void *item);

/*
 * \returns 1 if something was dequeued. 0 otherwise.
 */
int core_block_queue_dequeue(struct core_block_queue *self, void *item);

int core_block_queue_empty(struct core_block_queue *self);
int core_block_queue_full(struct core_block_queue *self);

int core_block_queue_size(struct core_block_queue *self);

void core_block_queue_set_memory_pool(struct core_block_queue *self,
                struct core_memory_pool *pool);

void core_block_queue_print(struct core_block_queue *self);
int core_block_queue_capacity(struct core_block_queue *self);

#endif
