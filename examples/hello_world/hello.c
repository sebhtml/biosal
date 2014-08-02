
#include "hello.h"

#include <stdio.h>

struct bsal_script hello_script = {
    .name = HELLO_SCRIPT,
    .init = hello_init,
    .destroy = hello_destroy,
    .receive = hello_receive,
    .size = sizeof(struct hello),
    .description = "hello"
};

void hello_init(struct bsal_actor *actor)
{
    struct hello *hello1;

    hello1 = (struct hello *)bsal_actor_concrete_actor(actor);
    bsal_vector_init(&hello1->initial_helloes, sizeof(int));
}

void hello_destroy(struct bsal_actor *actor)
{
    struct hello *hello1;

    hello1 = (struct hello *)bsal_actor_concrete_actor(actor);

    bsal_vector_destroy(&hello1->initial_helloes);
}

void hello_receive(struct bsal_actor *actor, struct bsal_message *message)
{
    int tag;
    int name;
    void *buffer;
    struct hello *hello1;
    int i;

    hello1 = (struct hello *)bsal_actor_concrete_actor(actor);
    tag = bsal_message_tag(message);
    name = bsal_actor_name(actor);
    buffer = bsal_message_buffer(message);

    if (tag == BSAL_ACTOR_START) {

        bsal_vector_unpack(&hello1->initial_helloes, buffer);

        printf("Hello world ! my name is actor:%d and I have %d acquaintances:",
                        name, (int)bsal_vector_size(&hello1->initial_helloes));

        for (i = 0; i < bsal_vector_size(&hello1->initial_helloes); i++) {
            printf(" actor:%d", bsal_vector_helper_at_as_int(&hello1->initial_helloes, i));
        }
        printf("\n");

        bsal_actor_helper_send_to_self_empty(actor, BSAL_ACTOR_STOP);
    }
}
