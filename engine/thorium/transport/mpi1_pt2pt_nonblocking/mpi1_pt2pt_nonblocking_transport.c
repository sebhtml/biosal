
#include "mpi1_pt2pt_nonblocking_transport.h"
#include "mpi1_request.h"

#include <performance/tracepoints/tracepoints.h>

#include <engine/thorium/transport/transport.h>

#include <engine/thorium/worker_buffer.h>
#include <engine/thorium/message.h>

#include <engine/thorium/configuration.h>

#include <core/system/memory.h>
#include <core/system/memory_pool.h>
#include <core/system/debugger.h>

#include <string.h>

/*
#define DEBUG_MPI1_PT2PT

#define DEBUG_BIG_HANDSHAKE
*/

/*
 * Use a dummy tag since the tag is actually stored inside the buffer
 * to avoid the MPI_TAG_UB bug / limitation in MPI.
 */
#define TAG_BIG_NO_VALUE (-9999)
#define TAG_SMALL_PAYLOAD 0
#define TAG_BIG_HANDSHAKE 1

/*
 * MPI_Comm_get_attr sets the pointer value to the attribute.
 *
 * \see https://svn.mcs.anl.gov/repos/mpi/mpich2/trunk/test/mpi/attr/baseattrcomm.c
 */
#define GET_ATTR_ACTUALLY_WORKS

/*
 * Each big message transported between a pair of ranks A and B needs to have its own tag.
 * Otherwise, a Irecv request may pull the wrong big message and this
 * generates a "The buffer is truncated !" message.
 */
#define TAG_BIG_START_VALUE 2

void thorium_mpi1_pt2pt_nonblocking_transport_init(struct thorium_transport *self, int *argc, char ***argv);
void thorium_mpi1_pt2pt_nonblocking_transport_destroy(struct thorium_transport *self);

int thorium_mpi1_pt2pt_nonblocking_transport_send(struct thorium_transport *self, struct thorium_message *message);
int thorium_mpi1_pt2pt_nonblocking_transport_receive(struct thorium_transport *self, struct thorium_message *message);

int thorium_mpi1_pt2pt_nonblocking_transport_test(struct thorium_transport *self, struct thorium_worker_buffer *worker_buffer);

void thorium_mpi1_pt2pt_nonblocking_transport_add_receive_request(struct thorium_transport *self, int tag, int count, int source);

int thorium_mpi1_pt2pt_nonblocking_transport_get_big_tag(struct thorium_transport *self);


struct thorium_transport_interface thorium_mpi1_pt2pt_nonblocking_transport_implementation = {
    .name = "mpi1_pt2pt_nonblocking_transport",
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
    int flag;
    int *value;

    concrete_self = thorium_transport_get_concrete_transport(self);

    concrete_self->maximum_buffer_size = 8192;
    concrete_self->maximum_receive_request_count = 64;

    /*
     * Avoid a problem with MPICH:
     * MPICH is hanging inside MPI_Test is there are too many requests.
     *
     * Here is the stack when MPICH hangs event if MPI_Test is supposed to be
     * non-blocking.
     *
     * (gdb) bt
#0  000000037e26df2f8 in poll () from /lib64/libc.so.6
#1  000007f5e624ebe7e in MPID_nem_tcp_connpoll () from /software/mpich/3.1.1-1/lib/libmpi.so.12
#2  000007f5e624db437 in MPIDI_CH3I_Progress () from /software/mpich/3.1.1-1/lib/libmpi.so.12
#3  000007f5e62455038 in MPIR_Test_impl () from /software/mpich/3.1.1-1/lib/libmpi.so.12
#4  000007f5e62455320 in PMPI_Test () from /software/mpich/3.1.1-1/lib/libmpi.so.12
#5  00000000000422556 in thorium_mpi1_request_test (self=0x7fff5efe3f70) at engine/thorium/transport/mpi1_pt2pt_nonblocking/mpi1_request.c:71
#6  00000000000421f42 in thorium_mpi1_pt2pt_nonblocking_transport_receive (self=0x7fff5efe4890, message=0x7fff5efe4040) at engine/thorium/transport/mpi1_pt2pt_nonblocking/mpi1_pt2pt_nonblocking_transport.c:328
#7  00000000000418bcd in thorium_node_run_loop (node=0x7fff5efe4110) at engine/thorium/node.c:1828
#8  00000000000418e9c in thorium_node_run (node=0x7fff5efe4110) at engine/thorium/node.c:714
#9  0000000000041e878 in biosal_thorium_engine_boot (argc=<value optimized out>, argv=<value optimized out>, script_identifier=-1359079029, script=0x6524c0) at engine/thorium/thorium_engine.c:30
#10 biosal_thorium_engine_boot_initial_actor (argc=<value optimized out>, argv=<value optimized out>, script_identifier=-1359079029, script=0x6524c0) at engine/thorium/thorium_engine.c:46
#11 00000000000416083 in main (argc=6, argv=0x7fff5efe5548) at applications/spate_metagenome_assembler/main.c:8
     */
#if 0
    if (self->size < 4) {
        concrete_self->maximum_receive_request_count = 4;
    }
#endif

    concrete_self->small_request_count = 0;

    concrete_self->maximum_big_receive_request_count = 8;
    concrete_self->big_request_count = 0;

    core_queue_init(&concrete_self->send_requests, sizeof(struct thorium_mpi1_request));
    core_queue_init(&concrete_self->receive_requests, sizeof(struct thorium_mpi1_request));

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

    concrete_self->current_big_tag = TAG_BIG_NO_VALUE;

    /*
     * Get the maximum value for a tag.
     * MPI_TAG_UB attribute value is at least 32767.
     *
     * \see http://stackoverflow.com/questions/9280671/mpi-tags-are-disabled
     */
    concrete_self->mpi_tag_ub = 32767;
    flag = 0;

#ifdef GET_ATTR_ACTUALLY_WORKS
    MPI_Comm_get_attr(MPI_COMM_WORLD, MPI_TAG_UB, &value, &flag);
#endif

    if (flag) {
        concrete_self->mpi_tag_ub = *value;
    }

#ifdef DISPLAY_MPI_TAG_UB
    if (self->rank == 0) {
        printf("Attribute value for MPI_TAG_UB is %d\n",
                        concrete_self->mpi_tag_ub);
        if (!flag) {
            printf("Attribute MPI_TAG_UB not found !\n");
        }
    }
#endif
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
    while (core_queue_dequeue(&concrete_self->send_requests, &active_request)) {
        MPI_Request_free(thorium_mpi1_request_request(&active_request));
    }

    core_queue_destroy(&concrete_self->send_requests);

    /*
     * Destroy receive requests.
     */
    while (core_queue_dequeue(&concrete_self->receive_requests, &active_request)) {
        buffer = thorium_mpi1_request_buffer(&active_request);
        core_memory_pool_free(self->inbound_message_memory_pool, buffer);
        MPI_Request_free(thorium_mpi1_request_request(&active_request));
    }

    core_queue_destroy(&concrete_self->receive_requests);

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

    tracepoint(thorium_message, transport_send_impl_enter, message);

    concrete_self = thorium_transport_get_concrete_transport(self);

    worker = thorium_message_worker(message);
    buffer = thorium_message_buffer(message);
    count = thorium_message_count(message);
    destination = thorium_message_destination_node(message);

    CORE_DEBUGGER_ASSERT(buffer == NULL || count > 0);

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

        payload_tag = thorium_mpi1_pt2pt_nonblocking_transport_get_big_tag(self);

        count2 = sizeof(payload_tag) + sizeof(count);
        buffer2 = core_memory_pool_allocate(self->outbound_message_memory_pool,
                        count2);

        core_memory_copy(buffer2 + 0, &payload_tag, sizeof(payload_tag));
        core_memory_copy(buffer2 + sizeof(payload_tag), &count, sizeof(count));

        thorium_mpi1_request_init(&active_request2, buffer2);
        thorium_mpi1_request_mark(&active_request2);

        request2 = thorium_mpi1_request_request(&active_request2);

        result = MPI_Isend(buffer2, count2, concrete_self->datatype, destination, TAG_BIG_HANDSHAKE,
                    concrete_self->communicator, request2);

#ifdef DEBUG_BIG_HANDSHAKE
        printf("DEBUG Sending TAG_BIG_HANDSHAKE count %d\n", count);
#endif

        if (result != MPI_SUCCESS) {
            return 0;
        }

        core_queue_enqueue(&concrete_self->send_requests, &active_request2);
    }

    thorium_mpi1_request_init_with_worker(&active_request, buffer, worker);

    request = thorium_mpi1_request_request(&active_request);

#ifdef DEBUG_MPI1_PT2PT
    printf("DEBUG Isend with tag %d RealTag %d\n", payload_tag,
                    thorium_message_tag(message));
#endif

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

    core_queue_enqueue(&concrete_self->send_requests, &active_request);

    tracepoint(thorium_message, transport_send_impl_exit, message);

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

        ++concrete_self->small_request_count;

#ifdef DEBUG_MPI1_PT2PT
        printf("DEBUG now has %d/%d TAG_SMALL_PAYLOAD requests\n", concrete_self->small_request_count,
                        concrete_self->maximum_receive_request_count);
#endif
    }

    /*
     * Make sure there are enough requests for big messages too.
     */
    if (concrete_self->big_request_count < concrete_self->maximum_big_receive_request_count) {

        /*
         * A buffer for a TAG_BIG_HANDSHAKE message
         * contains the tag and the count.
         */
        size = sizeof(int) * 2;
        request_tag = TAG_BIG_HANDSHAKE;
        thorium_mpi1_pt2pt_nonblocking_transport_add_receive_request(self, request_tag, size,
                        MPI_ANY_SOURCE);

        ++concrete_self->big_request_count;

#ifdef DEBUG_MPI1_PT2PT
        printf("DEBUG now has %d/%d TAG_BIG_HANDSHAKE requests\n",
                        concrete_self->big_request_count,
                        concrete_self->maximum_big_receive_request_count);
#endif
    }


    /*
     * Dequeue a request and check if it is ready.
     */
    if (!core_queue_dequeue(&concrete_self->receive_requests, &request)) {

        /*
         * There is nothing in the queue.
         */
        return 0;
    }

    /*
     * Test the receive request now.
     */

    if (!thorium_mpi1_request_test(&request)) {

        /*
         * Put it back in the queue now.
         */

        core_queue_enqueue(&concrete_self->receive_requests, &request);

        return 0;
    }

    /*
     * At this point, we have a request.
     */

    source = thorium_mpi1_request_source(&request);
    destination = thorium_transport_get_rank(self);
    tag = thorium_mpi1_request_tag(&request);
    count = thorium_mpi1_request_count(&request);
    buffer = thorium_mpi1_request_buffer(&request);

    thorium_mpi1_request_destroy(&request);

    /*
     * This is a big message
     */
    if (tag == TAG_BIG_HANDSHAKE) {

        request_tag = *(int *)(buffer + 0);
        size = *(int *)(buffer + sizeof(request_tag));

#ifdef DEBUG_BIG_HANDSHAKE
        printf("DEBUG received TAG_BIG_HANDSHAKE Tag: %d Count %d\n", request_tag, size);
#endif

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

        core_memory_pool_free(self->inbound_message_memory_pool, buffer);

        return 0;
    }

    /*
     * Prepare the message. The worker will be -1 to tell the thorium
     * code that this is not a worker buffer.
     */
    thorium_message_init_with_nodes(message, count, buffer, source, destination);

    tracepoint(thorium_message, transport_receive_impl, message);

#ifdef THORIUM_MPI1_PT2PT_NON_BLOCKING_DEBUG
    printf("DEBUG Non Blocking Test is conclusive Tag %d Count %d Buffer %p Source %d\n",
                    tag, count, buffer, source);
#endif

    if (tag >= TAG_BIG_START_VALUE && tag <= concrete_self->mpi_tag_ub) {
#ifdef DEBUG_BIG_HANDSHAKE
        printf("DEBUG Received TAG_BIG_PAYLOAD %d\n", count);
#endif
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
    int marked;

    concrete_self = thorium_transport_get_concrete_transport(self);

    if (core_queue_dequeue(&concrete_self->send_requests, &active_request)) {

        if (thorium_mpi1_request_test(&active_request)) {

            worker = thorium_mpi1_request_worker(&active_request);
            buffer = thorium_mpi1_request_buffer(&active_request);
            marked = thorium_mpi1_request_has_mark(&active_request);

            thorium_mpi1_request_destroy(&active_request);

            /*
             * This needs to remain inside this implementation, otherwise
             * this would be a leaky abstraction with a memory leak.
             */
            if (marked) {

#ifdef BUG_LEAKY_ABSTRACTION_2014_09_02
                printf("TAG_BIG_HANDSHAKE completed.\n");
#endif
                core_memory_pool_free(self->outbound_message_memory_pool, buffer);
                return 0;
            }

#ifdef BUG_LEAKY_ABSTRACTION_2014_09_02
            if (worker < 0) {
                printf("request completed for worker %d count %d\n", worker,
                                thorium_mpi1_request_count(&active_request));
            }
#endif

#ifdef CORE_DEBUGGER_ASSERT_ENABLED

#endif
            /*
             * Otherwise, the tag is either TAG_SMALL_PAYLOAD (fixed value)
             * or TAG_BIG_PAYLOAD (from TAG_BIG_START_VALUE to mpi_tag_ub)
             */
            thorium_worker_buffer_init(worker_buffer, worker, buffer);

            return 1;

        /* Just put it back in the FIFO for later */
        } else {
            core_queue_enqueue(&concrete_self->send_requests, &active_request);

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

    CORE_DEBUGGER_ASSERT(self->inbound_message_memory_pool != NULL);

    buffer = core_memory_pool_allocate(self->inbound_message_memory_pool,
                    count);

#ifdef DEBUG_BIG_HANDSHAKE
    if (tag == TAG_BIG_PAYLOAD) {
        printf("DEBUG add request for TAG_BIG_PAYLOAD count %d buffer %p\n",
                        count, buffer);
    }
#endif

    thorium_mpi1_request_init(&request, buffer);
    mpi_request = thorium_mpi1_request_request(&request);

    result = MPI_Irecv(buffer, count,
                    concrete_self->datatype, source,
                    tag, concrete_self->communicator, mpi_request);

    if (result != MPI_SUCCESS) {
        return;
    }

    core_queue_enqueue(&concrete_self->receive_requests, &request);

#ifdef THORIUM_MPI1_PT2PT_NON_BLOCKING_DEBUG
    printf("DEBUG Non Blocking added a request, now with %d/%d\n",
                        (int)core_queue_size(&concrete_self->receive_requests),
                        concrete_self->maximum_receive_request_count);
#endif
}

int thorium_mpi1_pt2pt_nonblocking_transport_get_big_tag(struct thorium_transport *self)
{
    struct thorium_mpi1_pt2pt_nonblocking_transport *concrete_self;
    int tag;

    concrete_self = thorium_transport_get_concrete_transport(self);

    /*
     * Start at beginning.
     */
    if (concrete_self->current_big_tag == TAG_BIG_NO_VALUE) {
        concrete_self->current_big_tag = TAG_BIG_START_VALUE;
    }

    tag = concrete_self->current_big_tag;

    ++concrete_self->current_big_tag;

    /*
     * Set the current tag to no value for the next
     * call.
     */
    if (concrete_self->current_big_tag > concrete_self->mpi_tag_ub) {
        concrete_self->current_big_tag = TAG_BIG_NO_VALUE;
    }

    return tag;
}
