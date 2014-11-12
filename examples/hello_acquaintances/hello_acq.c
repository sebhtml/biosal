
#include "hello_acq.h"

#include <stdio.h>

void hello_acq_init(struct thorium_actor *self);
void hello_acq_destroy(struct thorium_actor *self);
void hello_acq_receive(struct thorium_actor *self, struct thorium_message *message);

struct thorium_script hello_acq_script = {
    .identifier = SCRIPT_HELLO_ACQ,
    .init = hello_acq_init,
    .destroy = hello_acq_destroy,
    .receive = hello_acq_receive,
    .size = sizeof(struct hello),
    .name = "hello"
};

void hello_acq_init(struct thorium_actor *actor)
{
    struct hello *hello1;

    hello1 = (struct hello *)thorium_actor_concrete_actor(actor);
    core_vector_init(&hello1->initial_helloes, sizeof(int));
}

void hello_acq_destroy(struct thorium_actor *actor)
{
    struct hello *hello1;

    hello1 = (struct hello *)thorium_actor_concrete_actor(actor);

    core_vector_destroy(&hello1->initial_helloes);
}

void hello_acq_receive(struct thorium_actor *actor, struct thorium_message *message)
{
    int tag;
    int name;
    void *buffer;
    struct hello *hello1;
    int i;

    hello1 = (struct hello *)thorium_actor_concrete_actor(actor);
    tag = thorium_message_action(message);
    name = thorium_actor_name(actor);
    buffer = thorium_message_buffer(message);

    if (tag == ACTION_START) {

        core_vector_unpack(&hello1->initial_helloes, buffer);

        printf("Hello world ! my name is actor:%d and I have %d acquaintances:",
                        name, (int)core_vector_size(&hello1->initial_helloes));

        for (i = 0; i < core_vector_size(&hello1->initial_helloes); i++) {
            printf(" actor:%d", core_vector_at_as_int(&hello1->initial_helloes, i));
        }
        printf("\n");

        thorium_actor_send_to_self_empty(actor, ACTION_STOP);
    }
}
