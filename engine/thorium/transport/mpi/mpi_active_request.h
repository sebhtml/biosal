
#ifndef BSAL_MPI_ACTIVE_BUFFER_H
#define BSAL_MPI_ACTIVE_BUFFER_H

#include "mpi_transport.h"

struct thorium_active_request;

struct thorium_mpi_active_request {
#ifdef BSAL_TRANSPORT_USE_MPI
    MPI_Request request;
#else
    int mock;
#endif
};

void thorium_mpi_active_request_init(struct thorium_active_request *self);
void thorium_mpi_active_request_destroy(struct thorium_active_request *self);
int thorium_mpi_active_request_test(struct thorium_active_request *self);
void *thorium_mpi_active_request_request(struct thorium_active_request *self);

#endif
