
#include "active_buffer.h"

#include "transport.h"

#include "pami_active_buffer.h"
#include "mpi_active_buffer.h"

void bsal_active_buffer_init(struct bsal_active_buffer *active_buffer, void *buffer, int worker)
{
    int implementation;

#if defined(BSAL_TRANSPORT_USE_PAMI)
    implementation = BSAL_TRANSPORT_IMPLEMENTATION_PAMI;

#elif defined(BSAL_TRANSPORT_USE_MPI)
    implementation = BSAL_TRANSPORT_IMPLEMENTATION_MPI;

#else

    implementation = BSAL_TRANSPORT_IMPLEMENTATION_MOCK;
#endif

    active_buffer->buffer = buffer;
    active_buffer->worker = worker;

    if (implementation == BSAL_TRANSPORT_IMPLEMENTATION_PAMI) {
        active_buffer->active_buffer_init = bsal_pami_active_buffer_init;
        active_buffer->active_buffer_destroy = bsal_pami_active_buffer_destroy;
        active_buffer->active_buffer_test = bsal_pami_active_buffer_test;
        active_buffer->active_buffer_request = bsal_pami_active_buffer_request;

    } else if (implementation == BSAL_TRANSPORT_IMPLEMENTATION_MPI) {
        active_buffer->active_buffer_init = bsal_mpi_active_buffer_init;
        active_buffer->active_buffer_destroy = bsal_mpi_active_buffer_destroy;
        active_buffer->active_buffer_test = bsal_mpi_active_buffer_test;
        active_buffer->active_buffer_request = bsal_mpi_active_buffer_request;

    } else /* if (implementation == BSAL_TRANSPORT_IMPLEMENTATION_PAMI) */ {
        active_buffer->active_buffer_init = NULL;
        active_buffer->active_buffer_destroy = NULL;
        active_buffer->active_buffer_test = NULL;
        active_buffer->active_buffer_request = NULL;

    }

    active_buffer->active_buffer_init(active_buffer);
}

void bsal_active_buffer_destroy(struct bsal_active_buffer *active_buffer)
{
    active_buffer->active_buffer_destroy(active_buffer);

    active_buffer->buffer = NULL;
    active_buffer->worker = -1;

    active_buffer->active_buffer_init = NULL;
    active_buffer->active_buffer_destroy = NULL;
    active_buffer->active_buffer_test = NULL;
    active_buffer->active_buffer_request = NULL;
}

int bsal_active_buffer_test(struct bsal_active_buffer *active_buffer)
{
    return active_buffer->active_buffer_test(active_buffer);
}

void *bsal_active_buffer_buffer(struct bsal_active_buffer *active_buffer)
{
    return active_buffer->buffer;
}

void *bsal_active_buffer_request(struct bsal_active_buffer *active_buffer)
{
    return active_buffer->active_buffer_request(active_buffer);
}

int bsal_active_buffer_get_worker(struct bsal_active_buffer *active_buffer)
{
    return active_buffer->worker;
}

void *bsal_active_buffer_get_concrete_object(struct bsal_active_buffer *active_buffer)
{
    return (void *)&active_buffer->concrete_object;
}
