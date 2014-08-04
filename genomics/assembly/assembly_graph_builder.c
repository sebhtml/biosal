
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
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = bsal_actor_concrete_actor(self);

    bsal_vector_init(&concrete_self->spawners, sizeof(int));
    bsal_vector_init(&concrete_self->sequence_stores, sizeof(int));

    bsal_actor_register_handler(self, BSAL_ACTOR_START, bsal_assembly_graph_builder_start);
    bsal_actor_register_handler(self, BSAL_ACTOR_SET_PRODUCERS, bsal_assembly_graph_builder_set_producers);
    bsal_actor_register_handler(self, BSAL_ACTOR_ASK_TO_STOP, bsal_assembly_graph_builder_ask_to_stop);

}

void bsal_assembly_graph_builder_destroy(struct bsal_actor *self)
{
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = bsal_actor_concrete_actor(self);

    bsal_vector_destroy(&concrete_self->spawners);
    bsal_vector_destroy(&concrete_self->sequence_stores);
}

void bsal_assembly_graph_builder_receive(struct bsal_actor *self, struct bsal_message *message)
{
    bsal_actor_call_handler(self, message);
}

void bsal_assembly_graph_builder_ask_to_stop(struct bsal_actor *self, struct bsal_message *message)
{
    int name;

    name = bsal_actor_name(self);

    printf("builder/%d dies\n", name);

    bsal_actor_helper_ask_to_stop(self, message);
}

void bsal_assembly_graph_builder_start(struct bsal_actor *self, struct bsal_message *message)
{
    void *buffer;
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = bsal_actor_concrete_actor(self);
    printf("%s/%d received a urgent request from source %d to build an assembly graph using %d sequence stores\n",
                    bsal_actor_script_name(self),
                    bsal_actor_name(self),
                    bsal_message_source(message),
                    (int)bsal_vector_size(&concrete_self->sequence_stores));


    buffer = bsal_message_buffer(message);

    bsal_vector_unpack(&concrete_self->spawners, buffer);

    bsal_actor_helper_send_reply_empty(self, BSAL_ACTOR_START_REPLY);
}

void bsal_assembly_graph_builder_set_producers(struct bsal_actor *self, struct bsal_message *message)
{
    void *buffer;
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = bsal_actor_concrete_actor(self);
    buffer = bsal_message_buffer(message);

    bsal_vector_unpack(&concrete_self->sequence_stores, buffer);

    bsal_actor_helper_send_reply_empty(self, BSAL_ACTOR_SET_PRODUCERS_REPLY);

}
