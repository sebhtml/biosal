
#ifndef CORE_QUEUE_H
#define CORE_QUEUE_H

#include "vector.h"

struct core_queue {
    struct core_vector vector;
    int enqueue_index;
    int dequeue_index;
    int size;
    int bytes_per_element;
};

void core_queue_init(struct core_queue *self, int bytes_per_unit);
void core_queue_destroy(struct core_queue *self);

/*
 * \returns 1 if successful, 0 otherwise
 */
int core_queue_enqueue(struct core_queue *self, void *item);

/*
 * \returns 1 if something was dequeued. 0 otherwise.
 */
int core_queue_dequeue(struct core_queue *self, void *item);

int core_queue_empty(struct core_queue *self);
int core_queue_full(struct core_queue *self);

int core_queue_size(struct core_queue *self);

#endif
