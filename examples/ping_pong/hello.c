
#include "hello.h"

#include <core/helpers/integer.h>

#include <stdio.h>

void hello_init(struct thorium_actor *self);
void hello_destroy(struct thorium_actor *self);
void hello_receive(struct thorium_actor *self, struct thorium_message *message);

/* hello-hello_script */

struct thorium_script hello_script = {
    .identifier = SCRIPT_HELLO,
    .init = hello_init,
    .destroy = hello_destroy,
    .receive = hello_receive,
    .size = sizeof(struct hello),
    .name = "hello"
};

/* hello-hello_init */
void hello_init(struct thorium_actor *actor)
{
    struct hello *concrete_self;

    concrete_self = (struct hello *)thorium_actor_concrete_actor(actor);
    core_vector_init(&concrete_self->initial_helloes, sizeof(int));

}

/* hello-hello_destroy */

void hello_destroy(struct thorium_actor *actor)
{
    struct hello *concrete_self;

    concrete_self = (struct hello *)thorium_actor_concrete_actor(actor);

    core_vector_destroy(&concrete_self->initial_helloes);
}

#define BEHAVIOR(action_value) \
        if (action == action_value)

/* hello-hello_receive */
void hello_receive(struct thorium_actor *self, struct thorium_message *message)
{
    int tag;
    int name;
    void *buffer;
    struct hello *concrete_self;
    struct core_vector data_vector;
    int i;
    int action = thorium_message_action(message);

    concrete_self = thorium_actor_concrete_actor(self);
    name = thorium_actor_name(self);
    buffer = thorium_message_buffer(message);

    thorium_actor_log(self, "received action %d\n",
                    action);

    if (action == ACTION_START) {
        
        thorium_message_print(message);

        /*
         * Send a ping message to self.
         */
        thorium_actor_send_reply_empty(self, ACTION_MY_PING);

    }

    if (action == ACTION_MY_PING) {
        thorium_actor_send_reply_empty(self, ACTION_MY_PONG);

    }
    
    if (action == ACTION_MY_PONG) {

        thorium_actor_send_reply_empty(self, ACTION_ASK_TO_STOP);
    }

    if (action == ACTION_ASK_TO_STOP) {

        thorium_actor_send_reply_empty(self, ACTION_STOP);
    }
}

