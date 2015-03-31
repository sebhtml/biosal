
#include "tip_manager.h"

#include <core/helpers/integer.h>

#include <engine/thorium/actor.h>

#include <stdio.h>

void tip_manager_init(struct thorium_actor *self);
void tip_manager_destroy(struct thorium_actor *self);
void tip_manager_receive(struct thorium_actor *self, struct thorium_message *message);

struct thorium_script biosal_tip_manager_script = {
    .identifier = SCRIPT_TIP_MANAGER,
    .init = tip_manager_init,
    .destroy = tip_manager_destroy,
    .receive = tip_manager_receive,
    .size = sizeof(struct biosal_tip_manager),
    .name = "biosal_tip_manager",
};

void tip_manager_init(struct thorium_actor *self)
{
    struct biosal_tip_manager *concrete_self;
    concrete_self = thorium_actor_concrete_actor(self);
}

void tip_manager_destroy(struct thorium_actor *self)
{
    struct biosal_tip_manager *concrete_self;
    concrete_self = thorium_actor_concrete_actor(self);
}

void tip_manager_receive(struct thorium_actor *self, struct thorium_message *message)
{
    struct biosal_tip_manager *concrete_self;
    int action;
    void *buffer;
    int graph_manager_name;

    concrete_self = thorium_actor_concrete_actor(self);

    action = thorium_message_action(message);
    buffer = thorium_message_buffer(message);

    if (action == ACTION_START) {

        thorium_actor_log(self, "tip manager receives ACTION_START.\n");

        thorium_actor_log(self, "Removed tips !!");

        thorium_actor_send_reply_empty(self, ACTION_START_REPLY);

        /*
         * Also, kill self.
         */
    } else if (action == ACTION_ASK_TO_STOP) {

        thorium_actor_send_reply_empty(self, ACTION_ASK_TO_STOP_REPLY);

        thorium_actor_send_to_self_empty(self, ACTION_STOP);
    }
}

