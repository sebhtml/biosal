
#ifndef BSAL_MPI_ACTIVE_BUFFER_H
#define BSAL_MPI_ACTIVE_BUFFER_H

#include <mpi.h>

struct bsal_active_buffer;

struct bsal_mpi_active_buffer {
    MPI_Request request;
    void *buffer;
    int worker;
};

void bsal_mpi_active_buffer_init(struct bsal_active_buffer *active_buffer, void *buffer, int worker);
void bsal_mpi_active_buffer_destroy(struct bsal_active_buffer *active_buffer);
int bsal_mpi_active_buffer_test(struct bsal_active_buffer *active_buffer);
void *bsal_mpi_active_buffer_buffer(struct bsal_active_buffer *active_buffer);
MPI_Request *bsal_mpi_active_buffer_request(struct bsal_active_buffer *active_buffer);
int bsal_mpi_active_buffer_get_worker(struct bsal_active_buffer *active_buffer);

#endif
