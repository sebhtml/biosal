
#include "graph_store.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct bsal_script bsal_graph_store_script = {
    .name = BSAL_GRAPH_STORE_SCRIPT,
    .init = bsal_graph_store_init,
    .destroy = bsal_graph_store_destroy,
    .receive = bsal_graph_store_receive,
    .size = sizeof(struct bsal_graph_store)
};

void bsal_graph_store_init(struct bsal_actor *actor)
{
}

void bsal_graph_store_destroy(struct bsal_actor *actor)
{
}

void bsal_graph_store_receive(struct bsal_actor *actor, struct bsal_message *message)
{
    int tag;

    tag = bsal_message_tag(message);

    if (tag == BSAL_VERTEX) {

        bsal_actor_send_reply_empty(actor, BSAL_VERTEX_REPLY);

    } else if (tag == BSAL_GRAPH_STORE_STOP) {

        bsal_actor_send_to_self_empty(actor, BSAL_ACTOR_STOP);
    }
}
