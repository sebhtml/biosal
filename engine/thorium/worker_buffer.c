
#include "worker_buffer.h"

#include <stdlib.h>

void thorium_worker_buffer_init(struct thorium_worker_buffer *self, int worker, void *buffer)
{
    self->worker = worker;
    self->buffer = buffer;
}

void thorium_worker_buffer_destroy(struct thorium_worker_buffer *self)
{
    self->worker = -1;
    self->buffer = NULL;
}

int thorium_worker_buffer_get_worker(struct thorium_worker_buffer *self)
{
    return self->worker;
}

void *thorium_worker_buffer_get_buffer(struct thorium_worker_buffer *self)
{
    return self->buffer;
}


