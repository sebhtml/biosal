
#ifndef BSAL_FIFO_H
#define BSAL_FIFO_H

#include "vector.h"

struct bsal_queue {
    struct bsal_vector vector;
    int enqueue_index;
    int dequeue_index;
    int size;
    int bytes_per_element;
};

void bsal_queue_init(struct bsal_queue *self, int bytes_per_unit);
void bsal_queue_destroy(struct bsal_queue *self);

int bsal_queue_enqueue(struct bsal_queue *self, void *item);
int bsal_queue_dequeue(struct bsal_queue *self, void *item);

int bsal_queue_empty(struct bsal_queue *self);
int bsal_queue_full(struct bsal_queue *self);

int bsal_queue_size(struct bsal_queue *self);

#endif
