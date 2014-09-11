
#include "hello.h"

#include <stdio.h>

struct thorium_script hello_script = {
    .identifier = SCRIPT_HELLO,
    .init = hello_init,
    .destroy = hello_destroy,
    .receive = hello_receive,
    .size = sizeof(struct hello),
    .name = "hello"
};

void hello_init(struct thorium_actor *actor)
{
    struct hello *hello1;

    hello1 = (struct hello *)thorium_actor_concrete_actor(actor);
    bsal_vector_init(&hello1->initial_helloes, sizeof(int));
}

void hello_destroy(struct thorium_actor *actor)
{
    struct hello *hello1;

    hello1 = (struct hello *)thorium_actor_concrete_actor(actor);

    bsal_vector_destroy(&hello1->initial_helloes);
}

void hello_receive(struct thorium_actor *actor, struct thorium_message *message)
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

        bsal_vector_unpack(&hello1->initial_helloes, buffer);

        printf("Hello world ! my name is actor:%d and I have %d acquaintances:",
                        name, (int)bsal_vector_size(&hello1->initial_helloes));

        for (i = 0; i < bsal_vector_size(&hello1->initial_helloes); i++) {
            printf(" actor:%d", bsal_vector_at_as_int(&hello1->initial_helloes, i));
        }
        printf("\n");

        thorium_actor_send_to_self_empty(actor, ACTION_STOP);
    }
}
