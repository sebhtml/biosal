

#ifndef _BSAL_POOL
#define _BSAL_POOL

#include "bsal_work.h"

struct bsal_pool {
    struct bsal_work *ring;
    int ring_size;
    int producer_head;
    int consumer_head;
};

void bsal_pool_construct(struct bsal_pool *pool, int nodes, int threads);
void bsal_pool_destruct(struct bsal_pool *pool);

#endif
