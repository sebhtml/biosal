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
    int source;
    struct core_vector actors_to_greet;

    hello1 = (struct hello_acq *)thorium_actor_concrete_actor(actor);
    tag = thorium_message_action(message);
    name = thorium_actor_name(actor);
    buffer = thorium_message_buffer(message);
    source = thorium_message_source(message);


    if (tag == ACTION_START) {
        core_vector_unpack(&hello1->initial_helloes, buffer);

        printf("Hello world ! my name is actor:%d and I have %d acquaintances:",
                        name, (int)core_vector_size(&hello1->initial_helloes));

        /* This is just to print the acquaintances as in the original hello.c version. */
        for (i = 0; i < core_vector_size(&hello1->initial_helloes); i++) {
            printf(" actor:%d", core_vector_at_as_int(&hello1->initial_helloes, i));
            printf("\n");
        }

#if 0
        /* This is to send messages to myself and my peers. */

        for (i = 0; i < core_vector_size(&hello1->initial_helloes); i++) {
            if (name == core_vector_at_as_int(&hello1->initial_helloes, i)) {
                thorium_actor_send_to_self_empty(actor, ACTION_HELLO_ACQ_SELF);
            } else {
            	thorium_actor_send_int(actor, core_vector_at_as_int(&hello1->initial_helloes, i), ACTION_HELLO_ACQ_PEER, name);
            }
        }

#endif

        /* This is to send a vector of names to greet (everyone but myself) */

        struct core_vector actors_to_greet;
        core_vector_init(&actors_to_greet, sizeof(int));
        core_vector_push_back_int(&actors_to_greet, name);
        for (i = 0; i < core_vector_size(&hello1->initial_helloes); i++) {
            int acq_name = core_vector_at_as_int(&hello1->initial_helloes, i);
            if (name != acq_name) {
                core_vector_push_back(&actors_to_greet, &acq_name);
            }
        }
        thorium_actor_send_vector(actor, name, ACTION_HELLO_GREET_OTHERS, &actors_to_greet);

#if 0
    } else if (tag == ACTION_HELLO_ACQ_SELF) {
    	printf("Hello received from me (%d) to myself (%d)\n", source, name);
    } else if (tag == ACTION_HELLO_ACQ_PEER) {
    	printf("Hello received by %d from peer %d\n", name, source);
        thorium_actor_send_to_self_empty(actor, ACTION_STOP);
#endif
    } else if (tag == ACTION_HELLO_GREET_OTHERS) {
        printf("Hello received by %d from %d; done\n", name, source);
        core_vector_init(&actors_to_greet, sizeof(int));
        core_vector_unpack(&actors_to_greet, buffer);
        if (core_vector_size(&actors_to_greet) > 1) {
            int last_index = core_vector_size(&actors_to_greet);
            int next_to_greet = core_vector_at_as_int(&actors_to_greet, last_index-1);
            core_vector_resize(&actors_to_greet, last_index);
            thorium_actor_send_vector(actor, next_to_greet, ACTION_HELLO_GREET_OTHERS, &actors_to_greet);
        } else {
            int greeter = core_vector_at_as_int(&actors_to_greet, 0);
            thorium_actor_send_int(actor, greeter, ACTION_HELLO_DONE_GREETING_ALL, name);
        }
    } else if (tag == ACTION_HELLO_DONE_GREETING_ALL) {
        printf("The last acquaintance of %d was greeted by %d; done\n", name, source);
        thorium_actor_send_to_self_empty(actor, ACTION_STOP);
    }
}
