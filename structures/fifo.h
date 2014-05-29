
#ifndef _BSAL_FIFO_H
#define _BSAL_FIFO_H

#include "fifo_array.h"

struct bsal_fifo {
    struct bsal_fifo_array *first;
    struct bsal_fifo_array *last;
    struct bsal_fifo_array *bin;

    int units;
    int bytes_per_unit;
    volatile int size;
};

void bsal_fifo_init(struct bsal_fifo *fifo, int units, int bytes_per_unit);
void bsal_fifo_destroy(struct bsal_fifo *fifo);

int bsal_fifo_push(struct bsal_fifo *fifo, void *item);
int bsal_fifo_pop(struct bsal_fifo *fifo, void *item);

int bsal_fifo_bytes(struct bsal_fifo *fifo);
int bsal_fifo_empty(struct bsal_fifo *fifo);
int bsal_fifo_full(struct bsal_fifo *fifo);

struct bsal_fifo_array *bsal_fifo_get_array(struct bsal_fifo *fifo);
void bsal_fifo_delete(struct bsal_fifo_array *array);
void bsal_fifo_destroy_array(struct bsal_fifo_array *array);
int bsal_fifo_size(struct bsal_fifo *fifo);

#endif
