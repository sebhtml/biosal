
#include "active_buffer.h"

void bsal_active_buffer_init(struct bsal_active_buffer *active_buffer, void *buffer, int worker)
{
    bsal_mpi_active_buffer_init(active_buffer, buffer, worker);
}

void bsal_active_buffer_destroy(struct bsal_active_buffer *active_buffer)
{
    bsal_mpi_active_buffer_destroy(active_buffer);
}

int bsal_active_buffer_test(struct bsal_active_buffer *active_buffer)
{
    return bsal_mpi_active_buffer_test(active_buffer);
}

void *bsal_active_buffer_buffer(struct bsal_active_buffer *active_buffer)
{
    return bsal_mpi_active_buffer_buffer(active_buffer);
}

void *bsal_active_buffer_request(struct bsal_active_buffer *active_buffer)
{
    return bsal_mpi_active_buffer_request(active_buffer);
}

int bsal_active_buffer_get_worker(struct bsal_active_buffer *active_buffer)
{
    return bsal_mpi_active_buffer_get_worker(active_buffer);
}

void *bsal_active_buffer_get_concrete_object(struct bsal_active_buffer *active_buffer)
{
    return (void *)&active_buffer->concrete_object;
}
