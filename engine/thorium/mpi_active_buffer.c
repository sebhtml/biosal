
#include "mpi_active_buffer.h"

#include "active_buffer.h"

#include <stdlib.h>

void bsal_mpi_active_buffer_init(struct bsal_active_buffer *active_buffer, void *buffer, int worker)
{
    struct bsal_mpi_active_buffer *concrete_object;

    concrete_object = bsal_active_buffer_get_concrete_object(active_buffer);

    concrete_object->buffer = buffer;
    concrete_object->request = MPI_REQUEST_NULL;
    concrete_object->worker = worker;
}

/* \see http://blogs.cisco.com/performance/mpi_request_free-is-evil/
 * \see http://www.mpich.org/static/docs/v3.1/www3/MPI_Request_free.html
 */
void bsal_mpi_active_buffer_destroy(struct bsal_active_buffer *active_buffer)
{
    struct bsal_mpi_active_buffer *concrete_object;

    concrete_object = bsal_active_buffer_get_concrete_object(active_buffer);

    if (concrete_object->request != MPI_REQUEST_NULL) {
        MPI_Request_free(&concrete_object->request);
        concrete_object->request = MPI_REQUEST_NULL;
    }

    concrete_object->buffer = NULL;
}

/* \see http://www.mpich.org/static/docs/v3.1/www3/MPI_Test.html
 */
int bsal_mpi_active_buffer_test(struct bsal_active_buffer *active_buffer)
{
    struct bsal_mpi_active_buffer *concrete_object;

    concrete_object = bsal_active_buffer_get_concrete_object(active_buffer);

    MPI_Status status;
    int flag;
    int error;

    flag = 0;
    error = 0;

    error = MPI_Test(&concrete_object->request, &flag, &status);

    /* TODO do something with this error
     */
    if (error) {
        return 0;
    }

    if (flag) {
        concrete_object->request = MPI_REQUEST_NULL;
        return 1;
    }

    return 0;
}
void *bsal_mpi_active_buffer_buffer(struct bsal_active_buffer *active_buffer)
{
    struct bsal_mpi_active_buffer *concrete_object;

    concrete_object = bsal_active_buffer_get_concrete_object(active_buffer);

    return concrete_object->buffer;
}

MPI_Request *bsal_mpi_active_buffer_request(struct bsal_active_buffer *active_buffer)
{
    struct bsal_mpi_active_buffer *concrete_object;

    concrete_object = bsal_active_buffer_get_concrete_object(active_buffer);

    return &concrete_object->request;
}

int bsal_mpi_active_buffer_get_worker(struct bsal_active_buffer *active_buffer)
{
    struct bsal_mpi_active_buffer *concrete_object;

    concrete_object = bsal_active_buffer_get_concrete_object(active_buffer);

    return concrete_object->worker;
}
