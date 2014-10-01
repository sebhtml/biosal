
#ifndef BIOSAL_QUEUE_H
#define BIOSAL_QUEUE_H

#include "vector.h"

struct biosal_queue {
    struct biosal_vector vector;
    int enqueue_index;
    int dequeue_index;
    int size;
    int bytes_per_element;
};

void biosal_queue_init(struct biosal_queue *self, int bytes_per_unit);
void biosal_queue_destroy(struct biosal_queue *self);

/*
 * \returns 1 if successful, 0 otherwise
 */
int biosal_queue_enqueue(struct biosal_queue *self, void *item);

/*
 * \returns 1 if something was dequeued. 0 otherwise.
 */
int biosal_queue_dequeue(struct biosal_queue *self, void *item);

int biosal_queue_empty(struct biosal_queue *self);
int biosal_queue_full(struct biosal_queue *self);

int biosal_queue_size(struct biosal_queue *self);

#endif
