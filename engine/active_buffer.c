
#include "active_buffer.h"

#include <stdlib.h>

void bsal_active_buffer_init(struct bsal_active_buffer *self, void *buffer)
{
    self->buffer = buffer;
    self->request = MPI_REQUEST_NULL;
}

/* \see http://blogs.cisco.com/performance/mpi_request_free-is-evil/
 * \see http://www.mpich.org/static/docs/v3.1/www3/MPI_Request_free.html
 */
void bsal_active_buffer_destroy(struct bsal_active_buffer *self)
{
    if (self->request != MPI_REQUEST_NULL) {
        MPI_Request_free(&self->request);
        self->request = MPI_REQUEST_NULL;
    }

    self->buffer = NULL;
}

/* \see http://www.mpich.org/static/docs/v3.1/www3/MPI_Test.html
 */
int bsal_active_buffer_test(struct bsal_active_buffer *self)
{
    MPI_Status status;
    int flag;
    int error;

    flag = 0;
    error = 0;

    error = MPI_Test(&self->request, &flag, &status);

    /* TODO do something with this error
     */
    if (error) {
        return 0;
    }

    if (flag) {
        self->request = MPI_REQUEST_NULL;
        return 1;
    }

    return 0;
}
void *bsal_active_buffer_buffer(struct bsal_active_buffer *self)
{
    return self->buffer;
}

MPI_Request *bsal_active_buffer_request(struct bsal_active_buffer *self)
{
    return &self->request;
}
