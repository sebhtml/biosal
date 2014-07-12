
#include "ring.h"
#include "sender.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct bsal_script ring_script = {
    .name = RING_SCRIPT,
    .init = ring_init,
    .destroy = ring_destroy,
    .receive = ring_receive,
    .size = sizeof(struct ring),
    .description = "ring"
};

#define RING_STEP_RECEIVE_SPAWNERS 0
#define RING_STEP_SPAWN 1
#define RING_STEP_PUSH_NEXT 2

void ring_init(struct bsal_actor *actor)
{
    struct ring *concrete_actor;

    concrete_actor = (struct ring *)bsal_actor_concrete_actor(actor);

    concrete_actor->spawned_senders = 0;
    concrete_actor->senders = 100000;

#if 1
    concrete_actor->senders = 10000;
#endif
    concrete_actor->first = -1;
    concrete_actor->last = -1;
    concrete_actor->step = RING_STEP_RECEIVE_SPAWNERS;
    concrete_actor->ready_rings = 0;
    concrete_actor->ready_senders = 0;

    bsal_vector_init(&concrete_actor->spawners, sizeof(int));

    bsal_actor_add_script(actor, SENDER_SCRIPT, &sender_script);

    bsal_vector_init(&concrete_actor->spawners, 0);
}

void ring_destroy(struct bsal_actor *actor)
{
    struct ring *concrete_actor;

    concrete_actor = (struct ring *)bsal_actor_concrete_actor(actor);

    bsal_vector_destroy(&concrete_actor->spawners);
}

void ring_receive(struct bsal_actor *actor, struct bsal_message *message)
{
    int tag;
    int new_actor;
    int previous;
    int name;
    struct ring *concrete_actor;
    int messages;
    int previous_actor;
    char *buffer;
    int destination;

    concrete_actor = (struct ring *)bsal_actor_concrete_actor(actor);
    tag = bsal_message_tag(message);
    buffer = bsal_message_buffer(message);
    name = bsal_actor_get_name(actor);

    if (tag == BSAL_ACTOR_START) {

        bsal_vector_init(&concrete_actor->spawners, 0);
        bsal_vector_unpack(&concrete_actor->spawners, buffer);
        printf("actor %d BSAL_ACTOR_START, %d spawners\n", name,
                        (int)bsal_vector_size(&concrete_actor->spawners));

        destination = *(int *)bsal_vector_at(&concrete_actor->spawners, 0);
        bsal_actor_helper_send_empty(actor, destination, RING_READY);

    } else if (tag == RING_READY && concrete_actor->step == RING_STEP_RECEIVE_SPAWNERS) {

        concrete_actor->ready_rings++;

        if (concrete_actor->ready_rings == (int)bsal_vector_size(&concrete_actor->spawners)) {
            bsal_actor_helper_send_range_empty(actor, &concrete_actor->spawners, RING_SPAWN);
            concrete_actor->step = RING_STEP_SPAWN;
            concrete_actor->ready_rings = 0;
        }

    } else if (tag == RING_SPAWN) {

        printf("actor/%d is spawning %d senders\n",
                        bsal_actor_get_name(actor), concrete_actor->senders);

        concrete_actor->step = RING_STEP_SPAWN;

        new_actor = bsal_actor_spawn(actor, SENDER_SCRIPT);
        concrete_actor->first = new_actor;
        previous = new_actor;
        new_actor = bsal_actor_spawn(actor, SENDER_SCRIPT);
        concrete_actor->previous = new_actor;

        bsal_message_init(message, SENDER_SET_NEXT, sizeof(new_actor), &new_actor);
        bsal_actor_send(actor, previous, message);

        ++concrete_actor->spawned_senders;
        ++concrete_actor->spawned_senders;

    } else if (tag == RING_READY && concrete_actor->step == RING_STEP_SPAWN) {
        concrete_actor->ready_rings++;

#if 0
        printf("READY: %d/%d\n", concrete_actor->ready_rings, (int)bsal_vector_size(&concrete_actor->spawners));
#endif

        if (concrete_actor->ready_rings == bsal_vector_size(&concrete_actor->spawners)) {

            bsal_actor_helper_send_range_empty(actor, &concrete_actor->spawners, RING_PUSH_NEXT);
            concrete_actor->ready_rings = 0;
            concrete_actor->step = RING_STEP_PUSH_NEXT;
        }
    } else if (tag == RING_PUSH_NEXT) {

        previous_actor = bsal_vector_index_of(&concrete_actor->spawners, &name) - 1;
        if (previous_actor < 0) {
            previous_actor = bsal_vector_size(&concrete_actor->spawners)- 1;
        }

        printf("%d received RING_PUSH_NEXT\n", name);
        bsal_message_init(message, RING_SET_NEXT, sizeof(concrete_actor->first), &concrete_actor->first);
        bsal_actor_send(actor, *(int *)bsal_vector_at(&concrete_actor->spawners, previous_actor),
                       message);

    } else if (tag == RING_SET_NEXT) {

        concrete_actor->step = RING_STEP_PUSH_NEXT;
        bsal_message_set_tag(message, SENDER_SET_NEXT);
        bsal_actor_send(actor, concrete_actor->last, message);

    } else if (tag == SENDER_SET_NEXT_REPLY
                    && concrete_actor->step == RING_STEP_SPAWN) {


#if 0
        printf("ready senders %d/%d\n", concrete_actor->ready_senders, concrete_actor->senders);
#endif

        if (concrete_actor->spawned_senders % 10000 == 0) {
            printf("spawned %d/%d\n",
                        concrete_actor->spawned_senders,
                        concrete_actor->senders);
        }

        if (concrete_actor->spawned_senders == concrete_actor->senders) {

            printf("RING_STEP_SPAWN completed.\n");
            bsal_actor_helper_send_empty(actor, *(int *)bsal_vector_at(&concrete_actor->spawners, 0), RING_READY);
            concrete_actor->ready_senders = 0;

            concrete_actor->last = concrete_actor->previous;
        } else {
            new_actor = bsal_actor_spawn(actor, SENDER_SCRIPT);
            ++concrete_actor->spawned_senders;
            previous = concrete_actor->previous;

            bsal_message_init(message, SENDER_SET_NEXT, sizeof(new_actor), &new_actor);
            bsal_actor_send(actor, previous, message);

            concrete_actor->previous = new_actor;
        }
    } else if (tag == SENDER_SET_NEXT_REPLY
                    && concrete_actor->step == RING_STEP_PUSH_NEXT) {

        concrete_actor->ready_senders++;

        printf("SENDER_SET_NEXT_REPLY %d/%d\n",
                        concrete_actor->ready_senders,
                        1);

        if (concrete_actor->ready_senders == 1) {
            bsal_actor_helper_send_empty(actor, *(int *)bsal_vector_at(&concrete_actor->spawners, 0), RING_READY);
            printf("RING_STEP_PUSH_NEXT completed.\n");
            concrete_actor->ready_senders = 0;
        }


    } else if (tag == RING_READY && concrete_actor->step == RING_STEP_PUSH_NEXT) {
        concrete_actor->ready_rings++;

        if (concrete_actor->ready_rings == bsal_vector_size(&concrete_actor->spawners)) {

            printf("system is ready...\n");
            messages = 2000007;
            bsal_message_init(message, SENDER_HELLO, sizeof(messages), &messages);
            bsal_actor_send(actor, concrete_actor->first, message);

            concrete_actor->ready_rings = 0;
        }
    } else if (tag == SENDER_HELLO_REPLY) {

        bsal_actor_helper_send_range_empty(actor, &concrete_actor->spawners, RING_KILL);
        bsal_actor_helper_send_empty(actor, concrete_actor->first, SENDER_KILL);

    } else if (tag == RING_KILL) {

        bsal_actor_helper_send_to_self_empty(actor, BSAL_ACTOR_STOP);
    }
}
