
#include "assembly_graph_store.h"

#include <core/helpers/actor_helper.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct bsal_script bsal_assembly_graph_store_script = {
    .identifier = BSAL_ASSEMBLY_GRAPH_STORE_SCRIPT,
    .init = bsal_assembly_graph_store_init,
    .destroy = bsal_assembly_graph_store_destroy,
    .receive = bsal_assembly_graph_store_receive,
    .size = sizeof(struct bsal_assembly_graph_store),
    .name = "assembly_graph_store"
};

void bsal_assembly_graph_store_init(struct bsal_actor *self)
{
    bsal_actor_register_handler(self, BSAL_ACTOR_ASK_TO_STOP, bsal_assembly_graph_store_ask_to_stop);
}

void bsal_assembly_graph_store_destroy(struct bsal_actor *self)
{
}

void bsal_assembly_graph_store_receive(struct bsal_actor *self, struct bsal_message *message)
{
    bsal_actor_call_handler(self, message);
}

void bsal_assembly_graph_store_ask_to_stop(struct bsal_actor *self, struct bsal_message *message)
{
    bsal_actor_helper_ask_to_stop(self, message);
}
