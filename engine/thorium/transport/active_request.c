
#include "active_request.h"

#include "transport.h"

#include "pami/pami_active_request.h"
#include "mpi1_ptp/mpi1_ptp_active_request.h"

void thorium_active_request_init(struct thorium_active_request *self, void *buffer, int worker)
{
    int implementation;

#if defined(THORIUM_TRANSPORT_USE_PAMI)
    implementation = THORIUM_TRANSPORT_PAMI_IDENTIFIER;

#elif defined(THORIUM_TRANSPORT_USE_MPI1_PTP)
    implementation = THORIUM_TRANSPORT_MPI1_PTP_IDENTIFIER;

#else

    implementation = THORIUM_TRANSPORT_MOCK_IDENTIFIER;
#endif

    self->buffer = buffer;
    self->worker = worker;

    if (implementation == THORIUM_TRANSPORT_PAMI_IDENTIFIER) {
        self->active_request_init = thorium_pami_active_request_init;
        self->active_request_destroy = thorium_pami_active_request_destroy;
        self->active_request_test = thorium_pami_active_request_test;
        self->active_request_request = thorium_pami_active_request_request;

    } else if (implementation == THORIUM_TRANSPORT_MPI1_PTP_IDENTIFIER) {
        self->active_request_init = thorium_mpi1_ptp_active_request_init;
        self->active_request_destroy = thorium_mpi1_ptp_active_request_destroy;
        self->active_request_test = thorium_mpi1_ptp_active_request_test;
        self->active_request_request = thorium_mpi1_ptp_active_request_request;

    } else /* if (implementation == THORIUM_TRANSPORT_IMPLEMENTATION_PAMI) */ {
        self->active_request_init = NULL;
        self->active_request_destroy = NULL;
        self->active_request_test = NULL;
        self->active_request_request = NULL;

    }

    self->active_request_init(self);
}

void thorium_active_request_destroy(struct thorium_active_request *self)
{
    self->active_request_destroy(self);

    self->buffer = NULL;
    self->worker = -1;

    self->active_request_init = NULL;
    self->active_request_destroy = NULL;
    self->active_request_test = NULL;
    self->active_request_request = NULL;
}

int thorium_active_request_test(struct thorium_active_request *self)
{
    return self->active_request_test(self);
}

void *thorium_active_request_buffer(struct thorium_active_request *self)
{
    return self->buffer;
}

void *thorium_active_request_request(struct thorium_active_request *self)
{
    return self->active_request_request(self);
}

int thorium_active_request_get_worker(struct thorium_active_request *self)
{
    return self->worker;
}

void *thorium_active_request_get_concrete_object(struct thorium_active_request *self)
{
    return (void *)&self->concrete_object;
}
