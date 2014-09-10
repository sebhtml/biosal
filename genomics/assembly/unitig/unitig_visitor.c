
#include "unitig_visitor.h"

struct thorium_script bsal_unitig_visitor_script = {
    .identifier = SCRIPT_UNITIG_VISITOR,
    .name = "bsal_unitig_visitor",
    .init = bsal_unitig_visitor_init,
    .destroy = bsal_unitig_visitor_destroy,
    .receive = bsal_unitig_visitor_receive,
    .size = sizeof(struct bsal_unitig_visitor),
    .description = "A universe visitor for vertices."
};

void bsal_unitig_visitor_init(struct thorium_actor *self)
{
    struct bsal_unitig_visitor *concrete_self;

    concrete_self = (struct bsal_unitig_visitor *)thorium_actor_concrete_actor(self);
    bsal_vector_init(&concrete_self->graph_stores, sizeof(int));
}

void bsal_unitig_visitor_destroy(struct thorium_actor *self)
{
    struct bsal_unitig_visitor *concrete_self;
    concrete_self = (struct bsal_unitig_visitor *)thorium_actor_concrete_actor(self);

    bsal_vector_destroy(&concrete_self->graph_stores);
}

void bsal_unitig_visitor_receive(struct thorium_actor *self, struct thorium_message *message)
{
    int tag;
    void *buffer;
    struct bsal_unitig_visitor *concrete_self;

    tag = thorium_message_tag(message);
    buffer = thorium_message_buffer(message);
    concrete_self = (struct bsal_unitig_visitor *)thorium_actor_concrete_actor(self);

    if (tag == ACTION_START) {

        bsal_vector_unpack(&concrete_self->graph_stores, buffer);

        thorium_actor_send_reply_empty(self, ACTION_START_REPLY);
    }
}

