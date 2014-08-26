
#include "mpi1_pt2pt_active_request.h"

#include <stdlib.h>

void thorium_mpi1_pt2pt_active_request_init(struct thorium_mpi1_pt2pt_active_request *self,
                void *buffer, int worker)
{
    self->request = MPI_REQUEST_NULL;
    self->worker = worker;
    self->buffer = buffer;
}

/* \see http://blogs.cisco.com/performance/mpi_request_free-is-evil/
 * \see http://www.mpich.org/static/docs/v3.1/www3/MPI_Request_free.html
 */
void thorium_mpi1_pt2pt_active_request_destroy(struct thorium_mpi1_pt2pt_active_request *self)
{
    if (self->request != MPI_REQUEST_NULL) {
        MPI_Request_free(&self->request);
        self->request = MPI_REQUEST_NULL;
    }

    self->worker = -1;
    self->buffer = NULL;
}

/* \see http://www.mpich.org/static/docs/v3.1/www3/MPI_Test.html
 */
int thorium_mpi1_pt2pt_active_request_test(struct thorium_mpi1_pt2pt_active_request *self)
{
    MPI_Status status;
    int flag;
    int error;

    flag = 0;
    error = 0;

    error = MPI_Test(&self->request, &flag, &status);

    /* TODO do something with this error
     */
    if (error != MPI_SUCCESS) {
        return 0;
    }

    /*
     * The request is ready
     */
    if (flag) {
        self->request = MPI_REQUEST_NULL;
        return 1;
    }

    return 0;
}

void *thorium_mpi1_pt2pt_active_request_request(struct thorium_mpi1_pt2pt_active_request *self)
{
    return &self->request;
}

void *thorium_mpi1_pt2pt_active_request_buffer(struct thorium_mpi1_pt2pt_active_request *self)
{
    return self->buffer;
}

int thorium_mpi1_pt2pt_active_request_worker(struct thorium_mpi1_pt2pt_active_request *self)
{
    return self->worker;
}
