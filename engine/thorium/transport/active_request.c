
#include "active_request.h"

#include "transport.h"

#include "pami/pami_active_request.h"
#include "mpi/mpi_active_request.h"

void bsal_active_request_init(struct bsal_active_request *self, void *buffer, int worker)
{
    int implementation;

#if defined(BSAL_TRANSPORT_USE_PAMI)
    implementation = BSAL_TRANSPORT_PAMI_IDENTIFIER;

#elif defined(BSAL_TRANSPORT_USE_MPI)
    implementation = BSAL_TRANSPORT_MPI_IDENTIFIER;

#else

    implementation = BSAL_TRANSPORT_MOCK_IDENTIFIER;
#endif

    self->buffer = buffer;
    self->worker = worker;

    if (implementation == BSAL_TRANSPORT_PAMI_IDENTIFIER) {
        self->active_request_init = bsal_pami_active_request_init;
        self->active_request_destroy = bsal_pami_active_request_destroy;
        self->active_request_test = bsal_pami_active_request_test;
        self->active_request_request = bsal_pami_active_request_request;

    } else if (implementation == BSAL_TRANSPORT_MPI_IDENTIFIER) {
        self->active_request_init = bsal_mpi_active_request_init;
        self->active_request_destroy = bsal_mpi_active_request_destroy;
        self->active_request_test = bsal_mpi_active_request_test;
        self->active_request_request = bsal_mpi_active_request_request;

    } else /* if (implementation == BSAL_TRANSPORT_IMPLEMENTATION_PAMI) */ {
        self->active_request_init = NULL;
        self->active_request_destroy = NULL;
        self->active_request_test = NULL;
        self->active_request_request = NULL;

    }

    self->active_request_init(self);
}

void bsal_active_request_destroy(struct bsal_active_request *self)
{
    self->active_request_destroy(self);

    self->buffer = NULL;
    self->worker = -1;

    self->active_request_init = NULL;
    self->active_request_destroy = NULL;
    self->active_request_test = NULL;
    self->active_request_request = NULL;
}

int bsal_active_request_test(struct bsal_active_request *self)
{
    return self->active_request_test(self);
}

void *bsal_active_request_buffer(struct bsal_active_request *self)
{
    return self->buffer;
}

void *bsal_active_request_request(struct bsal_active_request *self)
{
    return self->active_request_request(self);
}

int bsal_active_request_get_worker(struct bsal_active_request *self)
{
    return self->worker;
}

void *bsal_active_request_get_concrete_object(struct bsal_active_request *self)
{
    return (void *)&self->concrete_object;
}
