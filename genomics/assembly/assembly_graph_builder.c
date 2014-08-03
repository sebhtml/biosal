
#include "assembly_graph_builder.h"

#include <core/helpers/actor_helper.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct bsal_script bsal_assembly_graph_builder_script = {
    .identifier = BSAL_ASSEMBLY_GRAPH_BUILDER_SCRIPT,
    .init = bsal_assembly_graph_builder_init,
    .destroy = bsal_assembly_graph_builder_destroy,
    .receive = bsal_assembly_graph_builder_receive,
    .size = sizeof(struct bsal_assembly_graph_builder),
    .name = "assembly_graph_builder"
};

void bsal_assembly_graph_builder_init(struct bsal_actor *self)
{
    bsal_actor_register(self, BSAL_ACTOR_ASK_TO_STOP, bsal_assembly_graph_builder_ask_to_stop);
}

void bsal_assembly_graph_builder_destroy(struct bsal_actor *self)
{
}

void bsal_assembly_graph_builder_receive(struct bsal_actor *self, struct bsal_message *message)
{
    bsal_actor_dispatch(self, message);
}

void bsal_assembly_graph_builder_ask_to_stop(struct bsal_actor *self, struct bsal_message *message)
{
    int name;

    name = bsal_actor_name(self);

    printf("builder/%d dies\n", name);

    bsal_actor_helper_ask_to_stop(self, message);
}
