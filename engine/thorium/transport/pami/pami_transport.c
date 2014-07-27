
#include "pami_transport.h"

#include <engine/thorium/transport/transport.h>
#include <engine/thorium/message.h>

void bsal_pami_transport_init(struct bsal_transport *transport, int *argc, char ***argv)
{
    struct bsal_pami_transport *pami_transport;

    pami_transport = bsal_transport_get_concrete_transport(transport);

    pami_transport->mock = -1;
    transport->rank = -1;
    transport->size = -1;
}

void bsal_pami_transport_destroy(struct bsal_transport *transport)
{
    struct bsal_pami_transport *pami_transport;

    pami_transport = bsal_transport_get_concrete_transport(transport);

    pami_transport->mock = -1;
    transport->rank = -1;
    transport->size = -1;
}

int bsal_pami_transport_send(struct bsal_transport *transport, struct bsal_message *message)
{
    int destination_node;
    void *buffer;

    destination_node = bsal_message_destination_node(message);
    buffer = bsal_message_buffer(message);


    /*
     * Send the data to the destination with PAMI
     */

    return 0;
}

int bsal_pami_transport_receive(struct bsal_transport *transport, struct bsal_message *message)
{
    return 0;
}


