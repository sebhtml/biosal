
#include "transport.h"

#include "message.h"
#include "node.h"

#include "active_buffer.h"

#include <core/system/debugger.h>

void bsal_transport_init(struct bsal_transport *transport, struct bsal_node *node,
                int *argc, char ***argv)
{
#ifdef BSAL_TRANSPORT_USE_PAMI
    transport->implementation = BSAL_TRANSPORT_IMPLEMENTATION_PAMI;

#elif defined(BSAL_TRANSPORT_USE_MPI)
    transport->implementation = BSAL_TRANSPORT_IMPLEMENTATION_MPI;

#else
    transport->implementation = BSAL_TRANSPORT_IMPLEMENTATION_MOCK;
#endif

    bsal_transport_set_functions(transport);

    transport->node = node;
    bsal_ring_queue_init(&transport->active_buffers, sizeof(struct bsal_active_buffer));
    transport->rank = -1;
    transport->size = -1;

    transport->transport_init(transport, argc, argv);

    BSAL_DEBUGGER_ASSERT(transport->rank >= 0);
    BSAL_DEBUGGER_ASSERT(transport->size >= 1);
    BSAL_DEBUGGER_ASSERT(transport->node != NULL);
}

void bsal_transport_destroy(struct bsal_transport *transport)
{
    struct bsal_active_buffer active_buffer;

    transport->transport_destroy(transport);

    while (bsal_ring_queue_dequeue(&transport->active_buffers, &active_buffer)) {
        bsal_active_buffer_destroy(&active_buffer);
    }

    bsal_ring_queue_destroy(&transport->active_buffers);

    transport->node = NULL;
    transport->rank = -1;
    transport->size = -1;
    transport->implementation = BSAL_TRANSPORT_IMPLEMENTATION_MOCK;

    bsal_transport_set_functions(transport);
}

int bsal_transport_send(struct bsal_transport *transport, struct bsal_message *message)
{
    return transport->transport_send(transport, message);
}

int bsal_transport_receive(struct bsal_transport *transport, struct bsal_message *message)
{
    return transport->transport_receive(transport, message);
}

void bsal_transport_resolve(struct bsal_transport *transport, struct bsal_message *message)
{
    int actor;
    int node_name;
    struct bsal_node *node;

    node = transport->node;

    actor = bsal_message_source(message);
    node_name = bsal_node_actor_node(node, actor);
    bsal_message_set_source_node(message, node_name);

    actor = bsal_message_destination(message);
    node_name = bsal_node_actor_node(node, actor);
    bsal_message_set_destination_node(message, node_name);
}

int bsal_transport_get_provided(struct bsal_transport *transport)
{
    return transport->provided;
}

int bsal_transport_get_rank(struct bsal_transport *transport)
{
    return transport->rank;
}

int bsal_transport_get_size(struct bsal_transport *transport)
{
    return transport->size;
}

int bsal_transport_test_requests(struct bsal_transport *transport, struct bsal_active_buffer *active_buffer)
{
    if (bsal_ring_queue_dequeue(&transport->active_buffers, active_buffer)) {

        if (bsal_active_buffer_test(active_buffer)) {

            return 1;

        /* Just put it back in the FIFO for later */
        } else {
            bsal_ring_queue_enqueue(&transport->active_buffers, active_buffer);

            return 0;
        }
    }

    return 0;
}

int bsal_transport_dequeue_active_buffer(struct bsal_transport *transport, struct bsal_active_buffer *active_buffer)
{
    return bsal_ring_queue_dequeue(&transport->active_buffers, active_buffer);
}

int bsal_transport_get_implementation(struct bsal_transport *transport)
{
    return transport->implementation;
}

void *bsal_transport_get_concrete_transport(struct bsal_transport *transport)
{
    return (void *)&transport->concrete_object;
}

void bsal_transport_set_functions(struct bsal_transport *transport)
{
    if (transport->implementation == BSAL_TRANSPORT_IMPLEMENTATION_PAMI) {
        bsal_transport_configure_pami(transport);

    } else if (transport->implementation == BSAL_TRANSPORT_IMPLEMENTATION_MPI) {

        bsal_transport_configure_mpi(transport);

    } else if (transport->implementation == BSAL_TRANSPORT_IMPLEMENTATION_MOCK) {

        bsal_transport_configure_mock(transport);

    } else {
        bsal_transport_configure_mock(transport);
    }
}

void bsal_transport_configure_pami(struct bsal_transport *transport)
{
    transport->transport_init = bsal_pami_transport_init;
    transport->transport_destroy = bsal_pami_transport_destroy;
    transport->transport_send = bsal_pami_transport_send;
    transport->transport_receive = bsal_pami_transport_receive;
}

void bsal_transport_configure_mpi(struct bsal_transport *transport)
{
    transport->transport_init = bsal_mpi_transport_init;
    transport->transport_destroy = bsal_mpi_transport_destroy;
    transport->transport_send = bsal_mpi_transport_send;
    transport->transport_receive = bsal_mpi_transport_receive;
}

void bsal_transport_configure_mock(struct bsal_transport *transport)
{
    transport->transport_init = NULL;
    transport->transport_destroy = NULL;
    transport->transport_send = NULL;
    transport->transport_receive = NULL;
}

void bsal_transport_prepare_received_message(struct bsal_transport *transport, struct bsal_message *message,
                int source, int tag, int count, void *buffer)
{
    int metadata_size;
    int destination;

    destination = transport->rank;
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
    bsal_transport_resolve(transport, message);

}
