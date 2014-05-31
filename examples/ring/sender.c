
#include "sender.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct bsal_actor_vtable sender_vtable = {
    .receive = sender_receive
};

void sender_init(struct bsal_actor *actor)
{
    struct sender *sender1;
/*
    int name;

    name = bsal_actor_name(actor);
    */
    sender1 = (struct sender *)bsal_actor_pointer(actor);
    sender1->received = 0;
    sender1->actors_per_node = 1000;

    /*
    printf("DEBUG actor %i sender_init\n", name);
    */
}

void sender_destroy(struct bsal_actor *actor)
{
    struct sender *sender1;

    /*
    printf("DEBUG sender_destroy\n");
    */
    sender1 = (struct sender *)bsal_actor_pointer(actor);
    sender1->received = 0;
    sender1->actors_per_node = 0;

    bsal_actor_die(actor);
}

void sender_receive(struct bsal_actor *actor, struct bsal_message *message)
{
    int tag;

    tag = bsal_message_tag(message);

    if (tag == BSAL_ACTOR_START) {
        sender_init(actor);

        sender_start(actor, message);


    } else if (tag == SENDER_HELLO) {

        if (bsal_actor_received_messages(actor) == 1) {
            sender_init(actor);
        }

        sender_hello(actor, message);

    } else if (tag == SENDER_KILL) {
        /*printf("Receives SENDER_KILL\n"); */

        sender_destroy(actor);

    } else if (tag == BSAL_ACTOR_BARRIER_REPLY) {

        /*printf("sender_receive BSAL_ACTOR_BARRIER_REPLY\n");*/

        if (bsal_actor_barrier_completed(actor)) {
            printf("Completed barrier !\n");
            sender_kill_all(actor, message);
        }
    }
}

void sender_kill_all(struct bsal_actor *actor, struct bsal_message *message)
{
    int total;
    int size;
    struct sender *sender1;
    sender1 = (struct sender *)bsal_actor_pointer(actor);

    size = bsal_actor_nodes(actor);
    total = size * sender1->actors_per_node;

    bsal_message_set_tag(message, SENDER_KILL);

    printf("Killing range %i to %i\n",
                    0, total - 1);

    bsal_actor_send_range(actor, 0, total - 1, message);
}

void sender_hello(struct bsal_actor *actor, struct bsal_message *message)
{
    int next;
    int total;
    int size;
    int name;
    struct sender *sender1;
    int events;

    /*printf("sender_hello\n"); */

    sender1 = (struct sender *)bsal_actor_pointer(actor);
    name = bsal_actor_name(actor);
    size = bsal_actor_nodes(actor);
    /*
    printf("DEBUG actor %i size: %i, actors_per_node: %i\n", name, size,
                    sender1->actors_per_node);
                    */
    total = size * sender1->actors_per_node;

    memcpy(&events, bsal_message_buffer(message), sizeof(events));
    events--;

    if (events == 0) {
        printf("sender_hello completed test on actor %i, %i actors in total\n",
                        name, total);

        printf("barrier %i-%i\n", 0, total - 1);
        bsal_actor_barrier(actor, 0, total - 1);
        /*sender_kill_all(actor, message); */
    } else {

        memcpy(bsal_message_buffer(message), &events, sizeof(events));
        next = (name + 1) % total;

        if (events % 20000 == 0) {
            printf("sender_hello remaining events %i\n", events);
        }

        /*printf("sender_hello send SENDER_HELLO\n");*/
        bsal_actor_send(actor, next, message);
    }
}

void sender_start(struct bsal_actor *actor, struct bsal_message *message)
{
    int name;
    int size;
    int i;
    int events;
    int total;
    int next;
    struct sender *sender1;

    printf("sender_start\n");

    sender1 = (struct sender *)bsal_actor_pointer(actor);

    /* \see http://rlrr.drum-corps.net/misc/primes3.shtml
     */
    events = 200087;
    /*events = 10000;*/

    name = bsal_actor_name(actor);
    size = bsal_actor_nodes(actor);
    total = size * sender1->actors_per_node;

    /* spawn a lot of actors ! */
    sender1->actors = (struct sender *)malloc((sender1->actors_per_node - 1) * sizeof(struct sender));

    for (i = 0; i < sender1->actors_per_node - 1; i++) {
        bsal_actor_spawn(actor, sender1->actors + i, &sender_vtable);
    }

    if (name != 0) {
        return;
    }

    printf("sender_start send SENDER_HELLO, system has: %i bsal_actors on %i "
                    "bsal_nodes (%i worker threads each)\n",
                    sender1->actors_per_node * bsal_actor_nodes(actor),
                    bsal_actor_nodes(actor),
                    bsal_actor_threads(actor));

    bsal_message_set_tag(message, SENDER_HELLO);
    bsal_message_set_buffer(message, &events);
    bsal_message_set_count(message, sizeof(events));

    next = (name + 1) % total;
    bsal_actor_send(actor, next, message);
}
