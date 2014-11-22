/*
 * hello_acq.c:
 *   A distributed version of Hello, World, works as follows:
 *     - An actor starts by having a list of acquaintances (one per node).
 *     - We use a vector to greet the other n-1 nodes.
 *       - vector @ 0 contains the actor who originated the greeting
 *       - vector @ 1..n-1 contains the actors to be greeted
 *     - The greet others action (ACTION_HELLO_GREET_OTHERS) pops the vector (if |vector| > 1)
 *       - vector is sent to the popped actor with the rest of the actors to be greeted
 *     - if |vector| == 1, that means we have greeted all of the acquaintances
 *       - send the done greeting all message to the originating actor
 *     - When the done greeting all message is received, we can stop.
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
    core_vector_init(&hello1->actors_to_greet, sizeof(int));
}

void hello_acq_destroy(struct thorium_actor *actor)
{
    struct hello_acq *hello1;

    hello1 = (struct hello_acq *)thorium_actor_concrete_actor(actor);
    core_vector_destroy(&hello1->initial_helloes);
    core_vector_destroy(&hello1->actors_to_greet);

}

void print_core_vector_int(struct core_vector *vector) {
    int i;
    for (i = 0; i < core_vector_size(vector); i++) {
        printf("@%d = %d; ", i, core_vector_at_as_int(vector, i));
    }
    printf("\n");
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

        printf("Hello world! My name is (actor) %d and I have %d acquaintances:\n",
                        name, (int)core_vector_size(&hello1->initial_helloes));

        print_core_vector_int(&hello1->initial_helloes);
        printf("\n");

        /* This creates a vector to greet the other n-1 initial acquaintances (besides myself) */
        core_vector_push_back_int(&hello1->actors_to_greet, name);
        for (i = 0; i < core_vector_size(&hello1->initial_helloes); i++) {
            int acq_name = core_vector_at_as_int(&hello1->initial_helloes, i);
            if (name != acq_name) {
                core_vector_push_back_int(&hello1->actors_to_greet, acq_name);
            }
        }
        printf("Actors to be greeted (except the first):\n");
        print_core_vector_int(&hello1->actors_to_greet);
        thorium_actor_send_vector(actor, name, ACTION_HELLO_GREET_OTHERS, &hello1->actors_to_greet);

    } else if (tag == ACTION_HELLO_GREET_OTHERS) {
        /* This passes on greetings by shriking the vector one greeted actor at a time until only the source remains */
        printf("Hello received by actor %d from actor %d\n", name, source);
        core_vector_unpack(&hello1->actors_to_greet, buffer);
        printf("Old List to Greet: ");
        print_core_vector_int(&hello1->actors_to_greet);
        if (core_vector_size(&hello1->actors_to_greet) > 1) {
            int size = core_vector_size(&hello1->actors_to_greet);
            int next_to_greet = core_vector_at_as_int(&hello1->actors_to_greet, size-1);
            core_vector_resize(&hello1->actors_to_greet, size-1);
            printf("New List (of remaining) to Greet: ");
            print_core_vector_int(&hello1->actors_to_greet);
            thorium_actor_send_vector(actor, next_to_greet, ACTION_HELLO_GREET_OTHERS, &hello1->actors_to_greet);
        } else {
            int greeter = core_vector_at_as_int(&hello1->actors_to_greet, 0);
            printf("Notifying Original Greeter = %d\n", greeter);
            thorium_actor_send_int(actor, greeter, ACTION_HELLO_DONE_GREETING_ALL, name);
        }
        printf("ACTION_HELLO_GREET_OTHERS - end\n");

    } else if (tag == ACTION_HELLO_DONE_GREETING_ALL) {
        printf("The original greeter %d was ack'd by %d\n", name, source);
        /* Need to check with Seb whether action stop stops everthing.
         * Termination detection issue with messages still in transit?
         */

        thorium_actor_send_to_self_empty(actor, ACTION_STOP);
    }
}
