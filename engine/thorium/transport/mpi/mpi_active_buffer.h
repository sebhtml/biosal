
#ifndef BSAL_MPI_ACTIVE_BUFFER_H
#define BSAL_MPI_ACTIVE_BUFFER_H

#include "mpi_transport.h"

struct bsal_active_buffer;

struct bsal_mpi_active_buffer {
#ifdef BSAL_TRANSPORT_USE_MPI
    MPI_Request request;
#else
    int mock;
#endif
};

void bsal_mpi_active_buffer_init(struct bsal_active_buffer *active_buffer);
void bsal_mpi_active_buffer_destroy(struct bsal_active_buffer *active_buffer);
int bsal_mpi_active_buffer_test(struct bsal_active_buffer *active_buffer);
void *bsal_mpi_active_buffer_request(struct bsal_active_buffer *active_buffer);

#endif
