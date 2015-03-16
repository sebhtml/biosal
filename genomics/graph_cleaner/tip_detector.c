
#include "tip_detector.h"

#include <core/helpers/integer.h>

#include <engine/thorium/actor.h>

void init(struct thorium_actor *self);
void destroy(struct thorium_actor *self);
void receive(struct thorium_actor *self, struct thorium_message *message);

struct thorium_script biosal_tip_detector_script = {
    .identifier = SCRIPT_TIP_DETECTOR,
    .init = init,
    .destroy = destroy,
    .receive = receive,
    .size = sizeof(struct biosal_tip_detector),
};

void init(struct thorium_actor *self)
{
    struct biosal_tip_detector *concrete_self;
    concrete_self = thorium_actor_concrete_actor(self);
}

void destroy(struct thorium_actor *self)
{
    struct biosal_tip_detector *concrete_self;
    concrete_self = thorium_actor_concrete_actor(self);
}

void receive(struct thorium_actor *self, struct thorium_message *message)
{
    struct biosal_tip_detector *concrete_self;
    int action;
    void *buffer;
    int graph_manager_name;

    concrete_self = thorium_actor_concrete_actor(self);

    action = thorium_message_action(message);
    buffer = thorium_message_buffer(message);

    if (action == ACTION_START) {

        core_int_unpack(&graph_manager_name, buffer);

        concrete_self->graph_manager_name = graph_manager_name;

        thorium_actor_send_to_self_empty(self, ACTION_STOP);
    }
}

