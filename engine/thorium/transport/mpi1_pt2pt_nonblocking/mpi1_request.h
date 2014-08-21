
#ifndef THORIUM_MPI1_REQUEST_H
#define THORIUM_MPI1_REQUEST_H

#include <mpi.h>

/*
 * An abstraction for a MPI 1.0 request.
 */
struct thorium_mpi1_request {

    MPI_Request request;
    int source;
    int tag;
    void *buffer;
    int worker;
    int count;
};

void thorium_mpi1_request_init(struct thorium_mpi1_request *self, void *buffer);
void thorium_mpi1_request_init_with_worker(struct thorium_mpi1_request *self, void *buffer, int worker);

void thorium_mpi1_request_destroy(struct thorium_mpi1_request *self);

int thorium_mpi1_request_count(struct thorium_mpi1_request *self);
int thorium_mpi1_request_source(struct thorium_mpi1_request *self);
int thorium_mpi1_request_tag(struct thorium_mpi1_request *self);
void *thorium_mpi1_request_buffer(struct thorium_mpi1_request *self);
int thorium_mpi1_request_worker(struct thorium_mpi1_request *self);
MPI_Request *thorium_mpi1_request_request(struct thorium_mpi1_request *self);
int thorium_mpi1_request_test(struct thorium_mpi1_request *self);

#endif
