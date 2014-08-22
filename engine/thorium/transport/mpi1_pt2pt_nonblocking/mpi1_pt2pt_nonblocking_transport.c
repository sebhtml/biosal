
#include "mpi1_pt2pt_nonblocking_transport.h"
#include "mpi1_request.h"

#include <engine/thorium/transport/transport.h>

#include <engine/thorium/worker_buffer.h>
#include <engine/thorium/message.h>

#include <core/system/memory.h>
#include <core/system/memory_pool.h>
#include <core/system/debugger.h>

#include <string.h>

/*
 * Use a dummy tag since the tag is actually stored inside the buffer
 * to avoid the MPI_TAG_UB bug / limitation in MPI.
 */
#define TAG_SMALL_PAYLOAD 10
#define TAG_BIG_NOTIFICATION 20
#define TAG_BIG_PAYLOAD 30

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

    /*
     * 128 MiB for receive buffers.
     */
    concrete_self = thorium_transport_get_concrete_transport(self);

    concrete_self->maximum_buffer_size = 8192;

    concrete_self->maximum_receive_request_count = 64;
    concrete_self->small_request_count = 0;

    concrete_self->maximum_big_receive_request_count = 4;
    concrete_self->big_request_count = 0;

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
    result = MPI_Comm_dup(MPI_COMM_WORLD, &concrete_self->communicator);

    if (result != MPI_SUCCESS) {
        return;
    }

    result = MPI_Comm_rank(concrete_self->communicator, &self->rank);

    if (result != MPI_SUCCESS) {
        return;
    }

    result = MPI_Comm_size(concrete_self->communicator, &self->size);

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
    void *buffer;

    concrete_self = thorium_transport_get_concrete_transport(self);

    /*
     * Destroy send requests.
     */
    while (bsal_ring_queue_dequeue(&concrete_self->send_requests, &active_request)) {
        MPI_Request_free(thorium_mpi1_request_request(&active_request));
    }

    bsal_ring_queue_destroy(&concrete_self->send_requests);

    /*
     * Destroy receive requests.
     */
    while (bsal_ring_queue_dequeue(&concrete_self->receive_requests, &active_request)) {
        buffer = thorium_mpi1_request_buffer(&active_request);
        bsal_memory_pool_free(self->inbound_message_memory_pool, buffer);
        MPI_Request_free(thorium_mpi1_request_request(&active_request));
    }

    bsal_ring_queue_destroy(&concrete_self->receive_requests);

    /*
     * \see http://www.mpich.org/static/docs/v3.1/www3/MPI_Comm_free.html
     */
    result = MPI_Comm_free(&concrete_self->communicator);

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
    char *buffer2;
    int count;
    int count2;
    int destination;
    MPI_Request *request;
    MPI_Request *request2;
    struct thorium_mpi1_request active_request;
    struct thorium_mpi1_request active_request2;
    int worker;
    int result;
    int payload_tag;

    concrete_self = thorium_transport_get_concrete_transport(self);

    worker = thorium_message_get_worker(message);
    buffer = thorium_message_buffer(message);
    count = thorium_message_count(message);
    destination = thorium_message_destination_node(message);

    BSAL_DEBUGGER_ASSERT(buffer == NULL || count > 0);

    payload_tag = TAG_SMALL_PAYLOAD;

    /*
     * If it is a large message, send a message to tell the destination
     * about it.
     */

    if (count > concrete_self->maximum_buffer_size) {

        /*
         * send a message to the other rank to tell it to do a MPI_Irecv with a bigger buffer
         * now.
         */

        count2 = sizeof(count);
        buffer2 = bsal_memory_pool_allocate(self->outbound_message_memory_pool,
                        count2);

        memcpy(buffer2, &count, count2);

        thorium_mpi1_request_init(&active_request2, buffer2);
        request2 = thorium_mpi1_request_request(&active_request2);

        result = MPI_Isend(buffer2, count2, concrete_self->datatype, destination, TAG_BIG_NOTIFICATION,
                    concrete_self->communicator, request2);

        printf("DEBUG Sending TAG_BIG_NOTIFICATION count %d\n", count);

        if (result != MPI_SUCCESS) {
            return 0;
        }

        bsal_ring_queue_enqueue(&concrete_self->send_requests, &active_request2);

        payload_tag = TAG_BIG_PAYLOAD;
    }

    thorium_mpi1_request_init_with_worker(&active_request, buffer, worker);
    request = thorium_mpi1_request_request(&active_request);

    /* get return value */
    result = MPI_Isend(buffer, count, concrete_self->datatype, destination, payload_tag,
                    concrete_self->communicator, request);

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
    void *buffer;
    int count;
    struct thorium_mpi1_request request;
    int source;
    int destination;
    int tag;
    int size;
    int request_tag;

    concrete_self = thorium_transport_get_concrete_transport(self);

    /*
     * If the number of requests is below the maximum, add some of them.
     */

    if (concrete_self->small_request_count < concrete_self->maximum_receive_request_count) {

        size = concrete_self->maximum_buffer_size;
        request_tag = TAG_SMALL_PAYLOAD;
        thorium_mpi1_pt2pt_nonblocking_transport_add_receive_request(self, request_tag, size,
                        MPI_ANY_SOURCE);
    }

    /*
     * Make sure there are enough requests for big messages too.
     */
    if (concrete_self->big_request_count < concrete_self->maximum_big_receive_request_count) {

        size = sizeof(int);
        request_tag = TAG_BIG_NOTIFICATION;
        thorium_mpi1_pt2pt_nonblocking_transport_add_receive_request(self, request_tag, size,
                        MPI_ANY_SOURCE);
    }

    /*
     * Dequeue a request and check if it is ready.
     */
    if (!bsal_ring_queue_dequeue(&concrete_self->receive_requests, &request)) {

        return 0;
    }

    /*
     * Test the receive request now.
     */
    if (!thorium_mpi1_request_test(&request)) {

        /*
         * Put it back in the queue now.
         */

        bsal_ring_queue_enqueue(&concrete_self->receive_requests, &request);

        return 0;
    }

    source = thorium_mpi1_request_source(&request);
    destination = thorium_transport_get_rank(self);
    tag = thorium_mpi1_request_tag(&request);
    count = thorium_mpi1_request_count(&request);
    buffer = thorium_mpi1_request_buffer(&request);

    thorium_mpi1_request_destroy(&request);

    /*
     * This is a big message
     */
    if (tag == TAG_BIG_NOTIFICATION) {

        size = *(int *)buffer;
        request_tag = TAG_BIG_PAYLOAD;

        printf("DEBUG received TAG_BIG_NOTIFICATION count %d\n", size);

        /*
         * Post the receive operation using the same source.
         *
         * This avoids this:
         *
         * Fatal error in PMPI_Test: Message truncated, error stack:
         * PMPI_Test(166)....: MPI_Test(request=0x7fffb3278350, flag=0x7fffb327831c, status=0x7fffb3278300) failed
         * MPIR_Test_impl(65):
         * do_cts(511).......: Message truncated; 117460 bytes received but buffer size is 9760
         */
        thorium_mpi1_pt2pt_nonblocking_transport_add_receive_request(self, request_tag, size,
                        source);

        bsal_memory_pool_free(self->inbound_message_memory_pool, buffer);

        return 0;
    }

    /*
     * Prepare the message. The worker will be -1 to tell the thorium
     * code that this is not a worker buffer.
     */
    thorium_message_init_with_nodes(message, tag, count, buffer, source,
                    destination);

#ifdef THORIUM_MPI1_PT2PT_NON_BLOCKING_DEBUG
    printf("DEBUG Non Blocking Test is conclusive Tag %d Count %d Buffer %p Source %d\n",
                    tag, count, buffer, source);
#endif

    if (tag == TAG_BIG_PAYLOAD) {
        printf("DEBUG Received TAG_BIG_PAYLOAD %d\n", count);
        --concrete_self->big_request_count;
    } else if (tag == TAG_SMALL_PAYLOAD) {
        --concrete_self->small_request_count;
    }

    /*
     * When the buffer is available again, it will be injected back into the
     * inbound_message_memory_pool.
     */

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

void thorium_mpi1_pt2pt_nonblocking_transport_add_receive_request(struct thorium_transport *self,
                int tag, int count, int source)
{
    void *buffer;
    struct thorium_mpi1_pt2pt_nonblocking_transport *concrete_self;
    struct thorium_mpi1_request request;
    MPI_Request *mpi_request;
    int result;

    concrete_self = thorium_transport_get_concrete_transport(self);

    buffer = bsal_memory_pool_allocate(self->inbound_message_memory_pool,
                    count);

    if (tag == TAG_BIG_PAYLOAD) {
        printf("DEBUG add request for TAG_BIG_PAYLOAD count %d buffer %p\n",
                        count, buffer);
    }
    thorium_mpi1_request_init(&request, buffer);
    mpi_request = thorium_mpi1_request_request(&request);

    result = MPI_Irecv(buffer, count,
                    concrete_self->datatype, source,
                    tag, concrete_self->communicator, mpi_request);

    bsal_ring_queue_enqueue(&concrete_self->receive_requests, &request);

    if (tag == TAG_BIG_NOTIFICATION) {
        ++concrete_self->big_request_count;
    } else if (tag == TAG_SMALL_PAYLOAD) {
        ++concrete_self->small_request_count;
    }

#ifdef THORIUM_MPI1_PT2PT_NON_BLOCKING_DEBUG
    printf("DEBUG Non Blocking added a request, now with %d/%d\n",
                        (int)bsal_ring_queue_size(&concrete_self->receive_requests),
                        concrete_self->maximum_receive_request_count);
#endif
}
