

#ifndef _BSAL_POOL
#define _BSAL_POOL

#include "bsal_fifo_array.h"

struct bsal_fifo {
    int units;
    int bytes_per_unit;
    struct bsal_fifo_array *first;
    struct bsal_fifo_array *last;
    struct bsal_fifo_array *bin;
};

void bsal_fifo_construct(struct bsal_fifo *fifo, int units, int bytes_per_unit);
void bsal_fifo_destruct(struct bsal_fifo *fifo);

int bsal_fifo_push(struct bsal_fifo *fifo, void *item);
int bsal_fifo_pop(struct bsal_fifo *fifo, void *item);

int bsal_fifo_bytes(struct bsal_fifo *fifo);
int bsal_fifo_empty(struct bsal_fifo *fifo);
int bsal_fifo_full(struct bsal_fifo *fifo);

struct bsal_fifo_array *bsal_fifo_get_array(struct bsal_fifo *fifo);
void bsal_fifo_delete(struct bsal_fifo_array *array);
void bsal_fifo_destroy_array(struct bsal_fifo_array *array);

#endif
