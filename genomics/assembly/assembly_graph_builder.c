
#include "assembly_graph_builder.h"

#include "assembly_graph_store.h"

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
    bsal_vector_init(&concrete_self->graph_stores, sizeof(int));

    bsal_actor_register_handler(self, BSAL_ACTOR_START, bsal_assembly_graph_builder_start);
    bsal_actor_register_handler(self, BSAL_ACTOR_SET_PRODUCERS, bsal_assembly_graph_builder_set_producers);
    bsal_actor_register_handler(self, BSAL_ACTOR_ASK_TO_STOP, bsal_assembly_graph_builder_ask_to_stop);
    bsal_actor_register_handler(self, BSAL_ACTOR_SPAWN_REPLY, bsal_assembly_graph_builder_spawn_reply);

    concrete_self->manager_for_graph_stores = BSAL_ACTOR_NOBODY;
    concrete_self->manager_for_classifier = BSAL_ACTOR_NOBODY;
    concrete_self->manager_for_classifier = BSAL_ACTOR_NOBODY;
}

void bsal_assembly_graph_builder_destroy(struct bsal_actor *self)
{
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = bsal_actor_concrete_actor(self);

    bsal_vector_destroy(&concrete_self->spawners);
    bsal_vector_destroy(&concrete_self->sequence_stores);
    bsal_vector_destroy(&concrete_self->graph_stores);

    concrete_self->manager_for_graph_stores = BSAL_ACTOR_NOBODY;
    concrete_self->manager_for_classifier = BSAL_ACTOR_NOBODY;
    concrete_self->manager_for_classifier = BSAL_ACTOR_NOBODY;
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
    int source;
    int spawner;

    source = bsal_message_source(message);
    concrete_self = bsal_actor_concrete_actor(self);

    if (source != concrete_self->source) {
        return;
    }

    buffer = bsal_message_buffer(message);

    bsal_vector_unpack(&concrete_self->spawners, buffer);

    printf("%s/%d received a urgent request from source %d to build an assembly graph using %d sequence stores (producers) and %d spawners\n",
                    bsal_actor_script_name(self),
                    bsal_actor_name(self),
                    source,
                    (int)bsal_vector_size(&concrete_self->sequence_stores),
                    (int)bsal_vector_size(&concrete_self->spawners));

    spawner = bsal_actor_get_spawner(self, &concrete_self->spawners);

    bsal_actor_helper_send_int(self, spawner, BSAL_ACTOR_SPAWN, BSAL_MANAGER_SCRIPT);
}

void bsal_assembly_graph_builder_spawn_reply(struct bsal_actor *self, struct bsal_message *message)
{
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = bsal_actor_concrete_actor(self);

    if (concrete_self->manager_for_graph_stores == BSAL_ACTOR_NOBODY) {
        bsal_message_helper_unpack_int(message, 0, &concrete_self->manager_for_graph_stores);

        bsal_actor_register_handler_with_source(self, BSAL_MANAGER_SET_SCRIPT_REPLY,
                        concrete_self->manager_for_graph_stores,
                        bsal_assembly_graph_builder_set_script_reply_store_manager);

        bsal_actor_register_handler_with_source(self, BSAL_ACTOR_START_REPLY,
                        concrete_self->manager_for_graph_stores,
                        bsal_assembly_graph_builder_start_reply_store_manager);

        bsal_actor_helper_send_int(self, concrete_self->manager_for_graph_stores, BSAL_MANAGER_SET_SCRIPT,
                        BSAL_ASSEMBLY_GRAPH_STORE_SCRIPT);
    }

}

void bsal_assembly_graph_builder_set_script_reply_store_manager(struct bsal_actor *self, struct bsal_message *message)
{
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = bsal_actor_concrete_actor(self);

    bsal_actor_helper_send_reply_vector(self, BSAL_ACTOR_START, &concrete_self->spawners);
}

void bsal_assembly_graph_builder_start_reply_store_manager(struct bsal_actor *self, struct bsal_message *message)
{
    void *buffer;
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = bsal_actor_concrete_actor(self);

    buffer = bsal_message_buffer(message);

    bsal_vector_unpack(&concrete_self->graph_stores, buffer);

    printf("%s/%d has %d graph stores for assembly\n",
                    bsal_actor_script_name(self),
                    bsal_actor_name(self),
                    (int)bsal_vector_size(&concrete_self->graph_stores));

    bsal_actor_helper_send_empty(self, concrete_self->source, BSAL_ACTOR_START_REPLY);
}

void bsal_assembly_graph_builder_set_producers(struct bsal_actor *self, struct bsal_message *message)
{
    void *buffer;
    struct bsal_assembly_graph_builder *concrete_self;
    int source;

    source = bsal_message_source(message);


    concrete_self = bsal_actor_concrete_actor(self);
    concrete_self->source = source;

    buffer = bsal_message_buffer(message);

    printf("%s/%d received a %d sequence stores from source %d\n",
                    bsal_actor_script_name(self),
                    bsal_actor_name(self),
                    (int)bsal_vector_size(&concrete_self->sequence_stores),
                    concrete_self->source);

    bsal_vector_unpack(&concrete_self->sequence_stores, buffer);

    bsal_actor_helper_send_reply_empty(self, BSAL_ACTOR_SET_PRODUCERS_REPLY);

}
