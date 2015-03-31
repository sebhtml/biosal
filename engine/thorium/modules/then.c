
#include "then.h"

#include <engine/thorium/actor.h>
#include <engine/thorium/message.h>

#include <stdio.h>

void thorium_actor_send_reply_then(struct thorium_actor *self,
                struct thorium_message *message, thorium_actor_receive_fn_t handler)
{
    int source = thorium_actor_source(self);

    thorium_actor_send_then(self, source, message, handler);
}

void thorium_actor_send_then(struct thorium_actor *self, int destination,
                struct thorium_message *message, thorium_actor_receive_fn_t handler)
{
    thorium_actor_send(self, destination, message);

    int parent_actor = thorium_message_source(message);
    int parent_message = thorium_message_get_identifier(message);

    /*
     * TODO: register the information so that the callback is invoked
     * when the response arrives.
     *
     * the key is parent_actor + parent_message and the value is handler.
     */

    thorium_actor_add_action_with_parent(self, parent_actor, parent_message,
                    handler);

    /*
    printf("NOT_IMPLEMENTED src %d dst %d msgid %d:%d" "\n",
                    thorium_actor_name(self),
                    destination,
                    parent_actor,
                    parent_message);
                    */
}



void thorium_actor_send_then_int(struct thorium_actor *actor, int destination, int action, int value,
                thorium_actor_receive_fn_t handler)
{
    struct thorium_message message;
    int count;
    int offset;

    count = sizeof(value);
    void *buffer = thorium_actor_allocate(actor, count);

    core_memory_copy(buffer, &value, sizeof(value));

    thorium_message_init(&message, action, count, buffer);
    thorium_actor_send_then(actor, destination, &message, handler);
    thorium_message_destroy(&message);
}


void thorium_actor_send_then_empty(struct thorium_actor *actor, int destination, int action,
                thorium_actor_receive_fn_t handler)
{
    struct thorium_message message;
    int offset;
    int count = 0;
    void *buffer = NULL;

    thorium_message_init(&message, action, count, buffer);
    thorium_actor_send_then(actor, destination, &message, handler);
    thorium_message_destroy(&message);
}
