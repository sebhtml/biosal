
#ifndef BSAL_ACTIVE_BUFFER_H
#define BSAL_ACTIVE_BUFFER_H

#include <mpi.h>

struct bsal_active_buffer {
    MPI_Request request;
    void *buffer;
};

void bsal_active_buffer_init(struct bsal_active_buffer *self, void *buffer);
void bsal_active_buffer_destroy(struct bsal_active_buffer *self);
int bsal_active_buffer_test(struct bsal_active_buffer *self);
void *bsal_active_buffer_buffer(struct bsal_active_buffer *self);
MPI_Request *bsal_active_buffer_request(struct bsal_active_buffer *self);

#endif
