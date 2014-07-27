
#include "mpi_transport.h"

#include <engine/thorium/transport/transport.h>
#include <engine/thorium/transport/active_buffer.h>

#include <engine/thorium/message.h>

#include <core/system/memory.h>
#include <core/system/debugger.h>

/*
 * \see http://www.mpich.org/static/docs/v3.1/www3/MPI_Comm_dup.html
 * \see http://www.dartmouth.edu/~rc/classes/intro_mpi/hello_world_ex.html
 * \see https://github.com/GeneAssembly/kiki/blob/master/ki.c#L960
 * \see http://mpi.deino.net/mpi_functions/MPI_Comm_create.html
 */
void bsal_mpi_transport_init(struct bsal_transport *transport, int *argc, char ***argv)
{
    int required;
    struct bsal_mpi_transport *mpi_transport;
    int result;

    mpi_transport = bsal_transport_get_concrete_transport(transport);

    /*
    required = MPI_THREAD_MULTIPLE;
    */
    required = MPI_THREAD_FUNNELED;

    result = MPI_Init_thread(argc, argv, required, &transport->provided);

    if (result != MPI_SUCCESS) {
        return;
    }

    /* make a new communicator for the library and don't use MPI_COMM_WORLD later */
    result = MPI_Comm_dup(MPI_COMM_WORLD, &mpi_transport->comm);

    if (result != MPI_SUCCESS) {
        return;
    }

    result = MPI_Comm_rank(mpi_transport->comm, &transport->rank);

    if (result != MPI_SUCCESS) {
        return;
    }

    result = MPI_Comm_size(mpi_transport->comm, &transport->size);

    if (result != MPI_SUCCESS) {
        return;
    }

    mpi_transport->datatype = MPI_BYTE;
}

void bsal_mpi_transport_destroy(struct bsal_transport *transport)
{
    struct bsal_mpi_transport *mpi_transport;
    int result;

    mpi_transport = bsal_transport_get_concrete_transport(transport);

    /*
     * \see http://www.mpich.org/static/docs/v3.1/www3/MPI_Comm_free.html
     */
    result = MPI_Comm_free(&mpi_transport->comm);

    if (result != MPI_SUCCESS) {
        return;
    }

    result = MPI_Finalize();

    if (result != MPI_SUCCESS) {
        return;
    }
}

/* \see http://www.mpich.org/static/docs/v3.1/www3/MPI_Isend.html */
int bsal_mpi_transport_send(struct bsal_transport *transport, struct bsal_message *message)
{
    struct bsal_mpi_transport *mpi_transport;

    char *buffer;
    int count;
    /* int source; */
    int destination;
    int tag;
    int metadata_size;
    MPI_Request *request;
    int all;
    struct bsal_active_buffer active_buffer;
    int worker;
    int result;

    mpi_transport = bsal_transport_get_concrete_transport(transport);

    worker = bsal_message_get_worker(message);
    buffer = bsal_message_buffer(message);
    count = bsal_message_count(message);
    metadata_size = bsal_message_metadata_size(message);
    all = count + metadata_size;
    destination = bsal_message_destination_node(message);
    tag = bsal_message_tag(message);

    bsal_active_buffer_init(&active_buffer, buffer, worker);
    request = bsal_active_buffer_request(&active_buffer);

    /* get return value */
    result = MPI_Isend(buffer, all, mpi_transport->datatype, destination, tag,
                    mpi_transport->comm, request);

    if (result != MPI_SUCCESS) {
        return 0;
    }

    /* store the MPI_Request to test it later to know when
     * the buffer can be reused
     */
    /* \see http://blogs.cisco.com/performance/mpi_request_free-is-evil/
     */
    /*MPI_Request_free(&request);*/

    bsal_ring_queue_enqueue(&transport->active_buffers, &active_buffer);

    return 1;
}

/* \see http://www.mpich.org/static/docs/v3.1/www3/MPI_Iprobe.html */
/* \see http://www.mpich.org/static/docs/v3.1/www3/MPI_Recv.html */
/* \see http://www.malcolmmclean.site11.com/www/MpiTutorial/MPIStatus.html */
int bsal_mpi_transport_receive(struct bsal_transport *transport, struct bsal_message *message)
{
    struct bsal_mpi_transport *mpi_transport;
    char *buffer;
    int count;
    int source;
    int destination;
    int tag;
    int flag;
    MPI_Status status;
    int result;

    mpi_transport = bsal_transport_get_concrete_transport(transport);
    source = MPI_ANY_SOURCE;
    tag = MPI_ANY_TAG;
    destination = transport->rank;

    /* get return value */
    result = MPI_Iprobe(source, tag, mpi_transport->comm, &flag, &status);

    if (result != MPI_SUCCESS) {
        return 0;
    }

    if (!flag) {
        return 0;
    }

    /* get return value */
    result = MPI_Get_count(&status, mpi_transport->datatype, &count);

    if (result != MPI_SUCCESS) {
        return 0;
    }

    /* TODO actually allocate (slab allocator) a buffer with count bytes ! */
    buffer = (char *)bsal_memory_allocate(count * sizeof(char));

    source = status.MPI_SOURCE;
    tag = status.MPI_TAG;

    /* TODO get return value */
    result = MPI_Recv(buffer, count, mpi_transport->datatype, source, tag,
                    mpi_transport->comm, &status);

    if (result != MPI_SUCCESS) {
        return 0;
    }

    bsal_transport_prepare_received_message(transport, message, source, tag, count, buffer);

    BSAL_DEBUGGER_ASSERT(result == MPI_SUCCESS);

    return 1;
}

