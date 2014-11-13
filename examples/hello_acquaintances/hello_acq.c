/*
 * hello_acq.c: Distributed Hello, World. Say hi to yourself and to your peers (first version).
 */

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
    .size = sizeof(struct hello_acq),
    .name = "hello"
};

void hello_acq_init(struct thorium_actor *actor)
{
    struct hello_acq *hello1;

    hello1 = (struct hello_acq *)thorium_actor_concrete_actor(actor);
    core_vector_init(&hello1->initial_helloes, sizeof(int));
}

void hello_acq_destroy(struct thorium_actor *actor)
{
    struct hello_acq *hello1;

    hello1 = (struct hello_acq *)thorium_actor_concrete_actor(actor);

    core_vector_destroy(&hello1->initial_helloes);
}

void hello_acq_receive(struct thorium_actor *actor, struct thorium_message *message)
{
    int tag;
    int name;
    void *buffer;
    struct hello_acq *hello1;
    int i;

    hello1 = (struct hello_acq *)thorium_actor_concrete_actor(actor);
    tag = thorium_message_action(message);
    name = thorium_actor_name(actor);
    buffer = thorium_message_buffer(message);

    if (tag == ACTION_START) {
        core_vector_unpack(&hello1->initial_helloes, buffer);

        printf("Hello world ! my name is actor:%d and I have %d acquaintances:",
                        name, (int)core_vector_size(&hello1->initial_helloes));

        /* This is just to print the acquaintances as in the original hello.c version. */
        for (i = 0; i < core_vector_size(&hello1->initial_helloes); i++) {
            printf(" actor:%d", core_vector_at_as_int(&hello1->initial_helloes, i));
            printf("\n");
        }

        /* This is to send messages to myself and my peers. */
        for (i = 0; i < core_vector_size(&hello1->initial_helloes); i++) {
            if (name == core_vector_at_as_int(&hello1->initial_helloes, i)) {
                thorium_actor_send_to_self_empty(actor, ACTION_HELLO_ACQ_SELF);
            } else {
            	thorium_actor_send_int(actor, core_vector_at_as_int(&hello1->initial_helloes, i), ACTION_HELLO_ACQ_PEER, name);
            }
        }

    } else if (tag == ACTION_HELLO_ACQ_SELF) {
    	int source = thorium_message_source(message);
    	printf("Hello received from me (%d) to myself (%d)\n", source, name);
    } else if (tag == ACTION_HELLO_ACQ_PEER) {
    	int source = thorium_message_source(message);
    	printf("Hello received by %d from peer %d\n", name, source);
        thorium_actor_send_to_self_empty(actor, ACTION_STOP);
    }
}
