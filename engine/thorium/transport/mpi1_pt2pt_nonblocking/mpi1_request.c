
#include "mpi1_request.h"

#include <stdlib.h>

void thorium_mpi1_request_init(struct thorium_mpi1_request *self, void *buffer)
{
    self->request = MPI_REQUEST_NULL;

    self->source = -1;
    self->count = -1;
    self->buffer = buffer;
    self->worker = -1;
    self->tag = -1;
}

void thorium_mpi1_request_destroy(struct thorium_mpi1_request *self)
{
    if (self->request != MPI_REQUEST_NULL) {
        MPI_Request_free(&self->request);

        self->request = MPI_REQUEST_NULL;
    }

    self->source = -1;
    self->tag = -1;
    self->buffer = NULL;
    self->worker = -1;
    self->count = -1;
}

void thorium_mpi1_request_init_with_worker(struct thorium_mpi1_request *self, void *buffer, int worker)
{
    thorium_mpi1_request_init(self, buffer);
    self->worker = worker;
}

int thorium_mpi1_request_source(struct thorium_mpi1_request *self)
{
    return self->source;
}

int thorium_mpi1_request_tag(struct thorium_mpi1_request *self)
{
    return self->tag;
}

void *thorium_mpi1_request_buffer(struct thorium_mpi1_request *self)
{
    return self->buffer;
}

int thorium_mpi1_request_worker(struct thorium_mpi1_request *self)
{
    return self->worker;
}

MPI_Request *thorium_mpi1_request_request(struct thorium_mpi1_request *self)
{
    return &self->request;
}

int thorium_mpi1_request_test(struct thorium_mpi1_request *self)
{
    MPI_Status status;
    int flag;
    int result;
    int count;

    flag = 0;
    result = MPI_Test(&self->request, &flag, &status);

    if (result != MPI_SUCCESS) {
        return 0;
    }

    if (!flag) {
        return 0;
    }

    result = MPI_Get_count(&status, MPI_BYTE, &count);

    if (result != MPI_SUCCESS) {
        return 0;
    }

    self->source = status.MPI_SOURCE;
    self->tag = status.MPI_TAG;
    self->count = count;

    return 1;
}

int thorium_mpi1_request_count(struct thorium_mpi1_request *self)
{
    return self->count;
}
