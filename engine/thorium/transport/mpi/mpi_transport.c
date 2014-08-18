
#include "mpi_transport.h"

#include <engine/thorium/transport/transport.h>
#include <engine/thorium/transport/active_request.h>

#include <engine/thorium/message.h>

#include <core/system/memory.h>
#include <core/system/memory_pool.h>
#include <core/system/debugger.h>

#define THORIUM_TRANSPORT_MPI_NAME "MPI: Message Passing Interface"

struct thorium_transport_interface thorium_mpi_transport_implementation = {
    .identifier = THORIUM_TRANSPORT_MPI_IDENTIFIER,
    .name = THORIUM_TRANSPORT_MPI_NAME,
    .size = sizeof(struct thorium_mpi_transport),
    .init = thorium_mpi_transport_init,
    .destroy = thorium_mpi_transport_destroy,
    .send = thorium_mpi_transport_send,
    .receive = thorium_mpi_transport_receive
};

/*
 * \see http://www.mpich.org/static/docs/v3.1/www3/MPI_Comm_dup.html
 * \see http://www.dartmouth.edu/~rc/classes/intro_mpi/hello_world_ex.html
 * \see https://github.com/GeneAssembly/kiki/blob/master/ki.c#L960
 * \see http://mpi.deino.net/mpi_functions/MPI_Comm_create.html
 */
void thorium_mpi_transport_init(struct thorium_transport *self, int *argc, char ***argv)
{
    int required;
    struct thorium_mpi_transport *concrete_self;
    int result;
    int provided;

    concrete_self = thorium_transport_get_concrete_transport(self);

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
        self->provided = THORIUM_THREAD_SINGLE;

    } else if (provided == MPI_THREAD_FUNNELED) {
        self->provided = THORIUM_THREAD_FUNNELED;

    } else if (provided == MPI_THREAD_SERIALIZED) {
        self->provided = THORIUM_THREAD_SERIALIZED;

    } else if (provided == MPI_THREAD_MULTIPLE) {
        self->provided = THORIUM_THREAD_MULTIPLE;
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

void thorium_mpi_transport_destroy(struct thorium_transport *self)
{
    struct thorium_mpi_transport *concrete_self;
    int result;

    concrete_self = thorium_transport_get_concrete_transport(self);

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
int thorium_mpi_transport_send(struct thorium_transport *self, struct thorium_message *message)
{
    struct thorium_mpi_transport *concrete_self;
    char *buffer;
    int count;
    int destination;
    int tag;
    MPI_Request *request;
    struct thorium_active_request active_request;
    int worker;
    int result;

    concrete_self = thorium_transport_get_concrete_transport(self);

    worker = thorium_message_get_worker(message);
    buffer = thorium_message_buffer(message);
    count = thorium_message_count(message);
    destination = thorium_message_destination_node(message);
    tag = thorium_message_tag(message);

    thorium_active_request_init(&active_request, buffer, worker);
    request = thorium_active_request_request(&active_request);

    BSAL_DEBUGGER_ASSERT(buffer == NULL || count > 0);

    /* get return value */
    result = MPI_Isend(buffer, count, concrete_self->datatype, destination, tag,
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
int thorium_mpi_transport_receive(struct thorium_transport *self, struct thorium_message *message)
{
    struct thorium_mpi_transport *concrete_self;
    char *buffer;
    int count;
    int source;
    int destination;
    int tag;
    int flag;
    MPI_Status status;
    int result;

    concrete_self = thorium_transport_get_concrete_transport(self);
    source = MPI_ANY_SOURCE;
    tag = MPI_ANY_TAG;

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

    destination = self->rank;

    /*
     * Prepare the message. The worker will be -1 to tell the thorium
     * code that this is not a worker buffer.
     */
    thorium_message_init_with_nodes(message, tag, count, buffer, source,
                    destination);

    return 1;
}

