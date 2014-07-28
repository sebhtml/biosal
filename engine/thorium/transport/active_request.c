
#include "active_request.h"

#include "transport.h"

#include "pami/pami_active_request.h"
#include "mpi/mpi_active_request.h"

void bsal_active_request_init(struct bsal_active_request *active_request, void *buffer, int worker)
{
    int implementation;

#if defined(BSAL_TRANSPORT_USE_PAMI)
    implementation = BSAL_TRANSPORT_PAMI_IDENTIFIER;

#elif defined(BSAL_TRANSPORT_USE_MPI)
    implementation = BSAL_TRANSPORT_MPI_IDENTIFIER;

#else

    implementation = BSAL_TRANSPORT_MOCK_IDENTIFIER;
#endif

    active_request->buffer = buffer;
    active_request->worker = worker;

    if (implementation == BSAL_TRANSPORT_PAMI_IDENTIFIER) {
        active_request->active_request_init = bsal_pami_active_request_init;
        active_request->active_request_destroy = bsal_pami_active_request_destroy;
        active_request->active_request_test = bsal_pami_active_request_test;
        active_request->active_request_request = bsal_pami_active_request_request;

    } else if (implementation == BSAL_TRANSPORT_MPI_IDENTIFIER) {
        active_request->active_request_init = bsal_mpi_active_request_init;
        active_request->active_request_destroy = bsal_mpi_active_request_destroy;
        active_request->active_request_test = bsal_mpi_active_request_test;
        active_request->active_request_request = bsal_mpi_active_request_request;

    } else /* if (implementation == BSAL_TRANSPORT_IMPLEMENTATION_PAMI) */ {
        active_request->active_request_init = NULL;
        active_request->active_request_destroy = NULL;
        active_request->active_request_test = NULL;
        active_request->active_request_request = NULL;

    }

    active_request->active_request_init(active_request);
}

void bsal_active_request_destroy(struct bsal_active_request *active_request)
{
    active_request->active_request_destroy(active_request);

    active_request->buffer = NULL;
    active_request->worker = -1;

    active_request->active_request_init = NULL;
    active_request->active_request_destroy = NULL;
    active_request->active_request_test = NULL;
    active_request->active_request_request = NULL;
}

int bsal_active_request_test(struct bsal_active_request *active_request)
{
    return active_request->active_request_test(active_request);
}

void *bsal_active_request_buffer(struct bsal_active_request *active_request)
{
    return active_request->buffer;
}

void *bsal_active_request_request(struct bsal_active_request *active_request)
{
    return active_request->active_request_request(active_request);
}

int bsal_active_request_get_worker(struct bsal_active_request *active_request)
{
    return active_request->worker;
}

void *bsal_active_request_get_concrete_object(struct bsal_active_request *active_request)
{
    return (void *)&active_request->concrete_object;
}
