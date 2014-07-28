
#ifndef BSAL_MPI_ACTIVE_BUFFER_H
#define BSAL_MPI_ACTIVE_BUFFER_H

#include "mpi_transport.h"

struct bsal_active_request;

struct bsal_mpi_active_request {
#ifdef BSAL_TRANSPORT_USE_MPI
    MPI_Request request;
#else
    int mock;
#endif
};

void bsal_mpi_active_request_init(struct bsal_active_request *active_request);
void bsal_mpi_active_request_destroy(struct bsal_active_request *active_request);
int bsal_mpi_active_request_test(struct bsal_active_request *active_request);
void *bsal_mpi_active_request_request(struct bsal_active_request *active_request);

#endif
