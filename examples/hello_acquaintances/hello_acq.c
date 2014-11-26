/*
 * hello_acq.c: 
 *   A distributed version of Hello, World, works as follows:
 *     - There are many possible ways to say a distributed "hello"
 *       - original hello.c is a great example: each of the initial actors says hello!
 *         only issue: no coordination involved, so it is more of a straight parallel example
 *     - this hello works by having the first actor in the list say hello to the other n-1 actors
 *       - a state vector of which of the n-1 peers has been greeted is sent from one actor to the next
 *       - when an actor is greeted, we indicate this by negating the (int) actor name. I would prefer to use
 *         a map but don't know whether we have pack/unpack yet. (It's also easier to send vectors in Thorium right now.)
 *       - when all of the n-1 peers have been greeted, each will have a negative entry in the state vector
 *       - when an actor is greeted, it has nothing left to do and can stop
 *       - the lone remaining actor will be the originator of the first hello message.
 * 
 */

#include "hello_acq.h"

#include <stdio.h>
#include <stdlib.h>

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
    core_vector_init(&hello1->actors_to_greet, sizeof(int));
}

void hello_acq_destroy(struct thorium_actor *actor)
{
    struct hello_acq *hello1;

    hello1 = (struct hello_acq *)thorium_actor_concrete_actor(actor);
    core_vector_destroy(&hello1->initial_helloes);
    core_vector_destroy(&hello1->actors_to_greet);

}

void hello_acq_receive(struct thorium_actor *actor, struct thorium_message *message)
{
    int tag;
    int name;
    void *buffer;
    struct hello_acq *hello1;
    int i;
    int source;

    hello1 = (struct hello_acq *)thorium_actor_concrete_actor(actor);
    tag = thorium_message_action(message);
    name = thorium_actor_name(actor);
    buffer = thorium_message_buffer(message);
    source = thorium_message_source(message);

    if (tag == ACTION_START) {
        core_vector_unpack(&hello1->initial_helloes, buffer);
        if (core_vector_at_as_int(&hello1->initial_helloes, 0) == name) {
            printf("START: Actor %d initiating greetings to all others.\n", name);
            core_vector_print_int(&hello1->initial_helloes);
            printf("\n");
            thorium_actor_send_vector(actor, name, ACTION_HELLO_GREET_OTHERS, &hello1->initial_helloes);
        }

    } else if (tag == ACTION_HELLO_GREET_OTHERS) {
        int actor_name = -1;

        printf("Hello received by actor %d from actor %d\n", name, source);
        core_vector_unpack(&hello1->actors_to_greet, buffer);
        printf("GREET_OTHERS: %d greeting others: ", name);
        core_vector_print_int(&hello1->actors_to_greet);
        printf("\n");

        /* replace with map that knows all actors that have been greeted */
        for (i = 1; i < core_vector_size(&hello1->actors_to_greet); i++) {
            actor_name = core_vector_at_as_int(&hello1->actors_to_greet, i);
            if (actor_name >= 0)
                break;
        }

        if (actor_name < 0) {
            printf("GREET_OTHERS: No actors left for %d to greet. This is the last one greeted!\n", name);
            int greeter = core_vector_at_as_int(&hello1->actors_to_greet, 0);
            /*
            thorium_actor_send_int(actor, greeter, ACTION_HELLO_DONE_GREETING_ALL, name);
            */
            thorium_actor_send_to_self_empty(actor, ACTION_HELLO_PEER_DONE);
            printf("\n");
        } else {
            core_vector_set_int(&hello1->actors_to_greet, i, -actor_name);  /* flip sign to indicate we greeted it. */
            core_vector_print_int(&hello1->actors_to_greet);
            printf("\n");
            thorium_actor_send_vector(actor, actor_name, ACTION_HELLO_GREET_OTHERS, &hello1->actors_to_greet);
            thorium_actor_send_to_self_empty(actor, ACTION_HELLO_PEER_DONE);
            printf("\n");
        }

    } else if (tag == ACTION_HELLO_PEER_DONE) {
        printf(">>> Peer %d is done\n\n", name);
        thorium_actor_send_to_self_empty(actor, ACTION_STOP);
    }
    /*
    } else if (tag == ACTION_HELLO_DONE_GREETING_ALL) {
        printf(">>>> The original greeter %d was ack'd by %d\n\n", name, source);
        thorium_actor_send_to_self_empty(actor, ACTION_STOP);
    }
    */
}
