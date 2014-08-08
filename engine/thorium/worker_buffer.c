
#include "worker_buffer.h"

#include <stdlib.h>

void bsal_worker_buffer_init(struct bsal_worker_buffer *self, int worker, void *buffer)
{
    self->worker = worker;
    self->buffer = buffer;
}

void bsal_worker_buffer_destroy(struct bsal_worker_buffer *self)
{
    self->worker = -1;
    self->buffer = NULL;
}

int bsal_worker_buffer_get_worker(struct bsal_worker_buffer *self)
{
    return self->worker;
}

void *bsal_worker_buffer_get_buffer(struct bsal_worker_buffer *self)
{
    return self->buffer;
}


