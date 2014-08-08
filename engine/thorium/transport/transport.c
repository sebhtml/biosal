
#include "transport.h"

#include "active_request.h"

#include <engine/thorium/worker_buffer.h>
#include <engine/thorium/message.h>
#include <engine/thorium/node.h>

#include <core/system/debugger.h>

void bsal_transport_init(struct bsal_transport *self, struct bsal_node *node,
                int *argc, char ***argv,
                struct bsal_memory_pool *inbound_message_memory_pool)
{
        /*
    printf("DEBUG Initiating transport\n");
    */
    /* Select the transport layer
     */
    bsal_transport_select(self);

    /*
     * Assign functions
     */
    bsal_transport_set(self);

    self->node = node;
    bsal_ring_queue_init(&self->active_requests, sizeof(struct bsal_active_request));

    self->rank = -1;
    self->size = -1;

    self->transport_init(self, argc, argv);

    BSAL_DEBUGGER_ASSERT(self->rank >= 0);
    BSAL_DEBUGGER_ASSERT(self->size >= 1);
    BSAL_DEBUGGER_ASSERT(self->node != NULL);

    self->inbound_message_memory_pool = inbound_message_memory_pool;
}

void bsal_transport_destroy(struct bsal_transport *self)
{
    struct bsal_active_request active_request;

    self->transport_destroy(self);

    while (bsal_ring_queue_dequeue(&self->active_requests, &active_request)) {
        bsal_active_request_destroy(&active_request);
    }

    bsal_ring_queue_destroy(&self->active_requests);

    self->node = NULL;
    self->rank = -1;
    self->size = -1;
    self->implementation = BSAL_TRANSPORT_IMPLEMENTATION_MOCK;

    bsal_transport_set(self);
}

int bsal_transport_send(struct bsal_transport *self, struct bsal_message *message)
{
    return self->transport_send(self, message);
}

int bsal_transport_receive(struct bsal_transport *self, struct bsal_message *message)
{
    return self->transport_receive(self, message);
}

void bsal_transport_resolve(struct bsal_transport *self, struct bsal_message *message)
{
    int actor;
    int node_name;
    struct bsal_node *node;

    node = self->node;

    actor = bsal_message_source(message);
    node_name = bsal_node_actor_node(node, actor);
    bsal_message_set_source_node(message, node_name);

    actor = bsal_message_destination(message);
    node_name = bsal_node_actor_node(node, actor);
    bsal_message_set_destination_node(message, node_name);
}

int bsal_transport_get_provided(struct bsal_transport *self)
{
    return self->provided;
}

int bsal_transport_get_rank(struct bsal_transport *self)
{
    return self->rank;
}

int bsal_transport_get_size(struct bsal_transport *self)
{
    return self->size;
}

int bsal_transport_test_requests(struct bsal_transport *self, struct bsal_worker_buffer *worker_buffer)
{
    struct bsal_active_request active_request;
    void *buffer;
    int worker;

    if (bsal_ring_queue_dequeue(&self->active_requests, &active_request)) {

        if (bsal_active_request_test(&active_request)) {

            worker = bsal_active_request_get_worker(&active_request);
            buffer = bsal_active_request_buffer(&active_request);

            bsal_worker_buffer_init(worker_buffer, worker, buffer);

            return 1;

        /* Just put it back in the FIFO for later */
        } else {
            bsal_ring_queue_enqueue(&self->active_requests, &active_request);

            return 0;
        }
    }

    return 0;
}

int bsal_transport_dequeue_active_request(struct bsal_transport *self, struct bsal_active_request *active_request)
{
    return bsal_ring_queue_dequeue(&self->active_requests, active_request);
}

int bsal_transport_get_implementation(struct bsal_transport *self)
{
    return self->implementation;
}

void *bsal_transport_get_concrete_transport(struct bsal_transport *self)
{
    return self->concrete_transport;
}

void bsal_transport_set(struct bsal_transport *self)
{
    if (self->implementation == BSAL_TRANSPORT_PAMI_IDENTIFIER) {
        bsal_transport_configure_pami(self);

    } else if (self->implementation == BSAL_TRANSPORT_MPI_IDENTIFIER) {

        bsal_transport_configure_mpi(self);

    } else if (self->implementation == BSAL_TRANSPORT_IMPLEMENTATION_MOCK) {

        bsal_transport_configure_mock(self);

    } else {
        bsal_transport_configure_mock(self);
    }
}

void bsal_transport_configure_pami(struct bsal_transport *self)
{
    self->concrete_transport = &self->pami_transport;

    self->transport_init = bsal_pami_transport_init;
    self->transport_destroy = bsal_pami_transport_destroy;
    self->transport_send = bsal_pami_transport_send;
    self->transport_receive = bsal_pami_transport_receive;
    self->transport_get_identifier = bsal_pami_transport_get_identifier;
    self->transport_get_name = bsal_pami_transport_get_name;
}

void bsal_transport_configure_mpi(struct bsal_transport *self)
{
    self->concrete_transport = &self->mpi_transport;

    self->transport_init = bsal_mpi_transport_init;
    self->transport_destroy = bsal_mpi_transport_destroy;
    self->transport_send = bsal_mpi_transport_send;
    self->transport_receive = bsal_mpi_transport_receive;
    self->transport_get_identifier = bsal_mpi_transport_get_identifier;
    self->transport_get_name = bsal_mpi_transport_get_name;
}

void bsal_transport_configure_mock(struct bsal_transport *self)
{
    self->concrete_transport = NULL;

    self->transport_init = NULL;
    self->transport_destroy = NULL;
    self->transport_send = NULL;
    self->transport_receive = NULL;
    self->transport_get_identifier = NULL;
    self->transport_get_name = NULL;
}

void bsal_transport_prepare_received_message(struct bsal_transport *self, struct bsal_message *message,
                int source, int tag, int count, void *buffer)
{
    int metadata_size;
    int destination;

    destination = self->rank;
    metadata_size = bsal_message_metadata_size(message);
    count -= metadata_size;

    /* Initially assign the MPI source rank and MPI destination
     * rank for the actor source and actor destination, respectively.
     * Then, read the metadata and resolve the MPI rank from
     * that. The resolved MPI ranks should be the same in all cases
     */
    bsal_message_init(message, tag, count, buffer);
    bsal_message_set_source(message, source);
    bsal_message_set_destination(message, destination);
    bsal_message_read_metadata(message);
    bsal_transport_resolve(self, message);
}

int bsal_transport_get_active_request_count(struct bsal_transport *self)
{
    return bsal_ring_queue_size(&self->active_requests);
}

int bsal_transport_get_identifier(struct bsal_transport *self)
{
    return self->transport_get_identifier(self);
}

const char *bsal_transport_get_name(struct bsal_transport *self)
{
    return self->transport_get_name(self);
}

void bsal_transport_select(struct bsal_transport *self)
{
    self->implementation = BSAL_TRANSPORT_MOCK_IDENTIFIER;

#if defined(BSAL_TRANSPORT_USE_PAMI)
    self->implementation = BSAL_TRANSPORT_PAMI_IDENTIFIER;

#elif defined(BSAL_TRANSPORT_USE_MPI)
    self->implementation = BSAL_TRANSPORT_MPI_IDENTIFIER;
#endif

    if (self->implementation == BSAL_TRANSPORT_MOCK_IDENTIFIER) {
        printf("Error: no transport implementation is available.\n");
        exit(1);
    }

    /*
    printf("DEBUG Transport is %d\n",
                    self->implementation);
                    */

}

void bsal_transport_print(struct bsal_transport *self)
{
    printf("%s TRANSPORT Rank: %d RankCount: %d Implementation: %s\n",
                    BSAL_NODE_THORIUM_PREFIX,
                self->rank, self->size,
                bsal_transport_get_name(self));
}
