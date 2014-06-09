
#ifndef BSAL_FIFO_H
#define BSAL_FIFO_H

#include "queue_group.h"

struct bsal_queue {
    struct bsal_queue_group *first;
    struct bsal_queue_group *last;
    struct bsal_queue_group *bin;

    int units;
    int bytes_per_unit;
    volatile int size;
};

void bsal_queue_init(struct bsal_queue *queue, int bytes_per_unit);
void bsal_queue_destroy(struct bsal_queue *queue);

int bsal_queue_enqueue(struct bsal_queue *queue, void *item);
int bsal_queue_dequeue(struct bsal_queue *queue, void *item);

int bsal_queue_bytes(struct bsal_queue *queue);
int bsal_queue_empty(struct bsal_queue *queue);
int bsal_queue_full(struct bsal_queue *queue);

struct bsal_queue_group *bsal_queue_get_array(struct bsal_queue *queue);
void bsal_queue_delete(struct bsal_queue_group *array);
void bsal_queue_destroy_array(struct bsal_queue_group *array);
int bsal_queue_size(struct bsal_queue *queue);

#endif
