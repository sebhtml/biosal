
#include "mpi_transport.h"

#include <engine/thorium/transport/transport.h>
#include <engine/thorium/transport/active_request.h>

#include <engine/thorium/message.h>

#include <core/system/memory.h>
#include <core/system/memory_pool.h>
#include <core/system/debugger.h>

/*
 * \see http://www.mpich.org/static/docs/v3.1/www3/MPI_Comm_dup.html
 * \see http://www.dartmouth.edu/~rc/classes/intro_mpi/hello_world_ex.html
 * \see https://github.com/GeneAssembly/kiki/blob/master/ki.c#L960
 * \see http://mpi.deino.net/mpi_functions/MPI_Comm_create.html
 */
void bsal_mpi_transport_init(struct bsal_transport *self, int *argc, char ***argv)
{
    int required;
    struct bsal_mpi_transport *concrete_self;
    int result;
    int provided;

    concrete_self = bsal_transport_get_concrete_transport(self);

    /*
    required = MPI_THREAD_MULTIPLE;
    */
    required = MPI_THREAD_FUNNELED;

    result = MPI_Init_thread(argc, argv, required, &provided);

    if (result != MPI_SUCCESS) {
        return;
    }

    /*
     * Set the provided level of thread support
     */
    if (provided == MPI_THREAD_SINGLE) {
        self->provided = BSAL_THREAD_SINGLE;

    } else if (provided == MPI_THREAD_FUNNELED) {
        self->provided = BSAL_THREAD_FUNNELED;

    } else if (provided == MPI_THREAD_SERIALIZED) {
        self->provided = BSAL_THREAD_SERIALIZED;

    } else if (provided == MPI_THREAD_MULTIPLE) {
        self->provided = BSAL_THREAD_MULTIPLE;
    }

    /* make a new communicator for the library and don't use MPI_COMM_WORLD later */
    result = MPI_Comm_dup(MPI_COMM_WORLD, &concrete_self->comm);

    if (result != MPI_SUCCESS) {
        return;
    }

    result = MPI_Comm_rank(concrete_self->comm, &self->rank);

    if (result != MPI_SUCCESS) {
        return;
    }

    result = MPI_Comm_size(concrete_self->comm, &self->size);

    if (result != MPI_SUCCESS) {
        return;
    }

    concrete_self->datatype = MPI_BYTE;
}

void bsal_mpi_transport_destroy(struct bsal_transport *self)
{
    struct bsal_mpi_transport *concrete_self;
    int result;

    concrete_self = bsal_transport_get_concrete_transport(self);

    /*
     * \see http://www.mpich.org/static/docs/v3.1/www3/MPI_Comm_free.html
     */
    result = MPI_Comm_free(&concrete_self->comm);

    if (result != MPI_SUCCESS) {
        return;
    }

    result = MPI_Finalize();

    if (result != MPI_SUCCESS) {
        return;
    }
}

/* \see http://www.mpich.org/static/docs/v3.1/www3/MPI_Isend.html */
int bsal_mpi_transport_send(struct bsal_transport *self, struct bsal_message *message)
{
    struct bsal_mpi_transport *concrete_self;

    char *buffer;
    int count;
    /* int source; */
    int destination;
    int tag;
    int metadata_size;
    MPI_Request *request;
    int all;
    struct bsal_active_request active_request;
    int worker;
    int result;

    concrete_self = bsal_transport_get_concrete_transport(self);

    worker = bsal_message_get_worker(message);
    buffer = bsal_message_buffer(message);
    count = bsal_message_count(message);
    metadata_size = bsal_message_metadata_size(message);
    all = count + metadata_size;
    destination = bsal_message_destination_node(message);
    tag = bsal_message_tag(message);

    bsal_active_request_init(&active_request, buffer, worker);
    request = bsal_active_request_request(&active_request);

    /* get return value */
    result = MPI_Isend(buffer, all, concrete_self->datatype, destination, tag,
                    concrete_self->comm, request);

    if (result != MPI_SUCCESS) {
        return 0;
    }

    /* store the MPI_Request to test it later to know when
     * the buffer can be reused
     */
    /* \see http://blogs.cisco.com/performance/mpi_request_free-is-evil/
     */
    /*MPI_Request_free(&request);*/

    bsal_ring_queue_enqueue(&self->active_requests, &active_request);

    return 1;
}

/* \see http://www.mpich.org/static/docs/v3.1/www3/MPI_Iprobe.html */
/* \see http://www.mpich.org/static/docs/v3.1/www3/MPI_Recv.html */
/* \see http://www.malcolmmclean.site11.com/www/MpiTutorial/MPIStatus.html */
int bsal_mpi_transport_receive(struct bsal_transport *self, struct bsal_message *message)
{
    struct bsal_mpi_transport *concrete_self;
    char *buffer;
    int count;
    int source;
    /*int destination;*/
    int tag;
    int flag;
    MPI_Status status;
    int result;

    concrete_self = bsal_transport_get_concrete_transport(self);
    source = MPI_ANY_SOURCE;
    tag = MPI_ANY_TAG;
    /*destination = self->rank;*/

    /* get return value */
    result = MPI_Iprobe(source, tag, concrete_self->comm, &flag, &status);

    if (result != MPI_SUCCESS) {
        return 0;
    }

    if (!flag) {
        return 0;
    }

    /* get return value */
    result = MPI_Get_count(&status, concrete_self->datatype, &count);

    if (result != MPI_SUCCESS) {
        return 0;
    }

    /* actually allocate (slab allocator) a buffer with count bytes ! */
    buffer = bsal_memory_pool_allocate(self->inbound_message_memory_pool,
                    count * sizeof(char));

    source = status.MPI_SOURCE;
    tag = status.MPI_TAG;

    /* get return value */
    result = MPI_Recv(buffer, count, concrete_self->datatype, source, tag,
                    concrete_self->comm, &status);

    if (result != MPI_SUCCESS) {
        return 0;
    }

    bsal_transport_prepare_received_message(self, message, source, tag, count, buffer);

    BSAL_DEBUGGER_ASSERT(result == MPI_SUCCESS);

    return 1;
}

int bsal_mpi_transport_get_identifier(struct bsal_transport *self)
{
    return BSAL_TRANSPORT_MPI_IDENTIFIER;
}

const char *bsal_mpi_transport_get_name(struct bsal_transport *self)
{
    return BSAL_TRANSPORT_MPI_NAME;
}
