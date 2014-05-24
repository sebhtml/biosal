

#ifndef _BSAL_POOL
#define _BSAL_POOL

#include "bsal_fifo_array.h"

struct bsal_fifo {
    struct bsal_fifo_array *first;
    struct bsal_fifo_array *last;
};

void bsal_fifo_construct(struct bsal_fifo *fifo);
void bsal_fifo_destruct(struct bsal_fifo *fifo);

#endif
