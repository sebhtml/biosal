
#include "sender.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void sender_init(struct thorium_actor *self);
void sender_destroy(struct thorium_actor *self);
void sender_receive(struct thorium_actor *self, struct thorium_message *message);

struct thorium_script sender_script = {
    .identifier = SCRIPT_SENDER,
    .init = sender_init,
    .destroy = sender_destroy,
    .receive = sender_receive,
    .size = sizeof(struct sender),
    .name = "sender"
};

void sender_init(struct thorium_actor *actor)
{
    struct sender *concrete_actor;

    concrete_actor = (struct sender *)thorium_actor_concrete_actor(actor);
    concrete_actor->next = -1;
}

void sender_destroy(struct thorium_actor *actor)
{
}

void sender_receive(struct thorium_actor *actor, struct thorium_message *message)
{
    int tag;
    void *buffer;
    struct sender *concrete_actor;
    int messages;
    int name;

    tag = thorium_message_action(message);
    buffer = thorium_message_buffer(message);
    concrete_actor = (struct sender *)thorium_actor_concrete_actor(actor);
    name = thorium_actor_name(actor);

    if (tag == ACTION_SENDER_SET_NEXT) {

        concrete_actor->next = *(int *)buffer;

        if (concrete_actor->next < 0) {
            printf("Error: invalid actor name\n");
        }
#if 0
        printf("receive ACTION_SENDER_SET_NEXT %d\n", concrete_actor->next);
#endif
        thorium_actor_send_reply_empty(actor, ACTION_SENDER_SET_NEXT_REPLY);

    } else if (tag == ACTION_SENDER_HELLO) {

        messages = *(int *)buffer;
        messages--;

        if (messages % 100003 == 0) {
            printf("actor %d says: %d\n", name, messages);

        }

        if (messages == 0) {
            thorium_actor_send_to_supervisor_empty(actor, ACTION_SENDER_HELLO_REPLY);
            return;
        }

        thorium_message_init(message, ACTION_SENDER_HELLO, sizeof(messages), &messages);
        thorium_actor_send(actor, concrete_actor->next, message);
/*
        printf("actor %d sends to next: actor %d\n", name, concrete_actor->next);
*/
    } else if (tag == ACTION_SENDER_KILL) {

        thorium_actor_send(actor, concrete_actor->next, message);

        thorium_actor_send_to_self_empty(actor, ACTION_STOP);
    }
}

