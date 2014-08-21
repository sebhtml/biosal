
#include "mpi1_pt2pt_nonblocking_transport.h"
#include "mpi1_request.h"

#include <engine/thorium/transport/transport.h>

#include <engine/thorium/worker_buffer.h>
#include <engine/thorium/message.h>

#include <core/system/memory.h>
#include <core/system/memory_pool.h>
#include <core/system/debugger.h>

/*
 * Use a dummy tag since the tag is actually stored inside the buffer
 * to avoid the MPI_TAG_UB bug / limitation in MPI.
 */
#define DUMMY_TAG 101

struct thorium_transport_interface thorium_mpi1_pt2pt_nonblocking_transport_implementation = {
    .name = "thorium_mpi1_pt2pt_nonblocking_transport_implementation",
    .size = sizeof(struct thorium_mpi1_pt2pt_nonblocking_transport),
    .init = thorium_mpi1_pt2pt_nonblocking_transport_init,
    .destroy = thorium_mpi1_pt2pt_nonblocking_transport_destroy,
    .send = thorium_mpi1_pt2pt_nonblocking_transport_send,
    .receive = thorium_mpi1_pt2pt_nonblocking_transport_receive,
    .test = thorium_mpi1_pt2pt_nonblocking_transport_test
};

/*
 * \see http://www.mpich.org/static/docs/v3.1/www3/MPI_Comm_dup.html
 * \see http://www.dartmouth.edu/~rc/classes/intro_mpi/hello_world_ex.html
 * \see https://github.com/GeneAssembly/kiki/blob/master/ki.c#L960
 * \see http://mpi.deino.net/mpi_functions/MPI_Comm_create.html
 */
void thorium_mpi1_pt2pt_nonblocking_transport_init(struct thorium_transport *self, int *argc, char ***argv)
{
    int required;
    struct thorium_mpi1_pt2pt_nonblocking_transport *concrete_self;
    int result;
    int provided;

    concrete_self = thorium_transport_get_concrete_transport(self);

    bsal_ring_queue_init(&concrete_self->send_requests, sizeof(struct thorium_mpi1_request));
    bsal_ring_queue_init(&concrete_self->receive_requests, sizeof(struct thorium_mpi1_request));

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

void thorium_mpi1_pt2pt_nonblocking_transport_destroy(struct thorium_transport *self)
{
    struct thorium_mpi1_pt2pt_nonblocking_transport *concrete_self;
    int result;
    struct thorium_mpi1_request active_request;

    concrete_self = thorium_transport_get_concrete_transport(self);

    while (bsal_ring_queue_dequeue(&concrete_self->send_requests, &active_request)) {
        MPI_Request_free(thorium_mpi1_request_request(&active_request));
    }

    while (bsal_ring_queue_dequeue(&concrete_self->receive_requests, &active_request)) {
        MPI_Request_free(thorium_mpi1_request_request(&active_request));
    }

    bsal_ring_queue_destroy(&concrete_self->send_requests);
    bsal_ring_queue_destroy(&concrete_self->receive_requests);

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
int thorium_mpi1_pt2pt_nonblocking_transport_send(struct thorium_transport *self, struct thorium_message *message)
{
    struct thorium_mpi1_pt2pt_nonblocking_transport *concrete_self;
    char *buffer;
    int count;
    int destination;
    int tag;
    MPI_Request *request;
    struct thorium_mpi1_request active_request;
    int worker;
    int result;

    concrete_self = thorium_transport_get_concrete_transport(self);

    worker = thorium_message_get_worker(message);
    buffer = thorium_message_buffer(message);
    count = thorium_message_count(message);
    destination = thorium_message_destination_node(message);
    tag = DUMMY_TAG;

    thorium_mpi1_request_init_with_worker(&active_request, buffer, worker);
    request = thorium_mpi1_request_request(&active_request);

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

    bsal_ring_queue_enqueue(&concrete_self->send_requests, &active_request);

    return 1;
}

/* \see http://www.mpich.org/static/docs/v3.1/www3/MPI_Iprobe.html */
/* \see http://www.mpich.org/static/docs/v3.1/www3/MPI_Recv.html */
/* \see http://www.malcolmmclean.site11.com/www/MpiTutorial/MPIStatus.html */
int thorium_mpi1_pt2pt_nonblocking_transport_receive(struct thorium_transport *self, struct thorium_message *message)
{
    struct thorium_mpi1_pt2pt_nonblocking_transport *concrete_self;
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
    tag = DUMMY_TAG;

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

int thorium_mpi1_pt2pt_nonblocking_transport_test(struct thorium_transport *self, struct thorium_worker_buffer *worker_buffer)
{
    struct thorium_mpi1_request active_request;
    struct thorium_mpi1_pt2pt_nonblocking_transport *concrete_self;
    void *buffer;
    int worker;

    concrete_self = thorium_transport_get_concrete_transport(self);

    if (bsal_ring_queue_dequeue(&concrete_self->send_requests, &active_request)) {

        if (thorium_mpi1_request_test(&active_request)) {

            worker = thorium_mpi1_request_worker(&active_request);
            buffer = thorium_mpi1_request_buffer(&active_request);

            thorium_worker_buffer_init(worker_buffer, worker, buffer);

            thorium_mpi1_request_destroy(&active_request);

            return 1;

        /* Just put it back in the FIFO for later */
        } else {
            bsal_ring_queue_enqueue(&concrete_self->send_requests, &active_request);

            return 0;
        }
    }

    return 0;
}

