
#include "mpi_active_request.h"

#include <engine/thorium/transport/active_request.h>

#include <stdlib.h>

void thorium_mpi_active_request_init(struct thorium_active_request *self)
{
    struct thorium_mpi_active_request *concrete_object;

    concrete_object = thorium_active_request_get_concrete_object(self);

    concrete_object->request = MPI_REQUEST_NULL;
}

/* \see http://blogs.cisco.com/performance/mpi_request_free-is-evil/
 * \see http://www.mpich.org/static/docs/v3.1/www3/MPI_Request_free.html
 */
void thorium_mpi_active_request_destroy(struct thorium_active_request *self)
{
    struct thorium_mpi_active_request *concrete_object;

    concrete_object = thorium_active_request_get_concrete_object(self);

    if (concrete_object->request != MPI_REQUEST_NULL) {
        MPI_Request_free(&concrete_object->request);
        concrete_object->request = MPI_REQUEST_NULL;
    }

}

/* \see http://www.mpich.org/static/docs/v3.1/www3/MPI_Test.html
 */
int thorium_mpi_active_request_test(struct thorium_active_request *self)
{
    struct thorium_mpi_active_request *concrete_object;

    concrete_object = thorium_active_request_get_concrete_object(self);

    MPI_Status status;
    int flag;
    int error;

    flag = 0;
    error = 0;

    error = MPI_Test(&concrete_object->request, &flag, &status);

    /* TODO do something with this error
     */
    if (error != MPI_SUCCESS) {
        return 0;
    }

    /*
     * The request is ready
     */
    if (flag) {
        concrete_object->request = MPI_REQUEST_NULL;
        return 1;
    }

    return 0;
}

void *thorium_mpi_active_request_request(struct thorium_active_request *self)
{
    struct thorium_mpi_active_request *concrete_object;

    concrete_object = thorium_active_request_get_concrete_object(self);

    return &concrete_object->request;
}

