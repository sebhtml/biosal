
#include "assembly_graph.h"

#include <core/helpers/actor_helper.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct bsal_script bsal_assembly_graph_script = {
    .identifier = BSAL_ASSEMBLY_GRAPH_SCRIPT,
    .init = bsal_assembly_graph_init,
    .destroy = bsal_assembly_graph_destroy,
    .receive = bsal_assembly_graph_receive,
    .size = sizeof(struct bsal_assembly_graph),
    .name = "assembly_graph"
};

void bsal_assembly_graph_init(struct bsal_actor *self)
{
    bsal_actor_register(self, BSAL_ACTOR_ASK_TO_STOP, bsal_assembly_graph_ask_to_stop);
}

void bsal_assembly_graph_destroy(struct bsal_actor *self)
{
}

void bsal_assembly_graph_receive(struct bsal_actor *self, struct bsal_message *message)
{
    bsal_actor_dispatch(self, message);
}

void bsal_assembly_graph_ask_to_stop(struct bsal_actor *self, struct bsal_message *message)
{
    bsal_actor_helper_ask_to_stop(self, message);
}
