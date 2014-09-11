
#include "ring.h"
#include "sender.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct thorium_script ring_script = {
    .identifier = SCRIPT_RING,
    .init = ring_init,
    .destroy = ring_destroy,
    .receive = ring_receive,
    .size = sizeof(struct ring),
    .name = "ring"
};

#define RING_STEP_RECEIVE_SPAWNERS 0
#define RING_STEP_SPAWN 1
#define RING_STEP_PUSH_NEXT 2

void ring_init(struct thorium_actor *actor)
{
    struct ring *concrete_actor;

    concrete_actor = (struct ring *)thorium_actor_concrete_actor(actor);

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

    thorium_actor_add_script(actor, SCRIPT_SENDER, &sender_script);

    bsal_vector_init(&concrete_actor->spawners, 0);
}

void ring_destroy(struct thorium_actor *actor)
{
    struct ring *concrete_actor;

    concrete_actor = (struct ring *)thorium_actor_concrete_actor(actor);

    bsal_vector_destroy(&concrete_actor->spawners);
}

void ring_receive(struct thorium_actor *actor, struct thorium_message *message)
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

    concrete_actor = (struct ring *)thorium_actor_concrete_actor(actor);
    tag = thorium_message_action(message);
    buffer = thorium_message_buffer(message);
    name = thorium_actor_name(actor);

    if (tag == ACTION_START) {

        bsal_vector_init(&concrete_actor->spawners, 0);
        bsal_vector_unpack(&concrete_actor->spawners, buffer);
        printf("actor %d ACTION_START, %d spawners\n", name,
                        (int)bsal_vector_size(&concrete_actor->spawners));

        destination = *(int *)bsal_vector_at(&concrete_actor->spawners, 0);
        thorium_actor_send_empty(actor, destination, ACTION_RING_READY);

    } else if (tag == ACTION_RING_READY && concrete_actor->step == RING_STEP_RECEIVE_SPAWNERS) {

        concrete_actor->ready_rings++;

        if (concrete_actor->ready_rings == (int)bsal_vector_size(&concrete_actor->spawners)) {
            thorium_actor_send_range_empty(actor, &concrete_actor->spawners, ACTION_RING_SPAWN);
            concrete_actor->step = RING_STEP_SPAWN;
            concrete_actor->ready_rings = 0;
        }

    } else if (tag == ACTION_RING_SPAWN) {

        printf("actor/%d is spawning %d senders\n",
                        thorium_actor_name(actor), concrete_actor->senders);

        concrete_actor->step = RING_STEP_SPAWN;

        new_actor = thorium_actor_spawn(actor, SCRIPT_SENDER);
        concrete_actor->first = new_actor;
        previous = new_actor;
        new_actor = thorium_actor_spawn(actor, SCRIPT_SENDER);
        concrete_actor->previous = new_actor;

        thorium_message_init(message, ACTION_SENDER_SET_NEXT, sizeof(new_actor), &new_actor);
        thorium_actor_send(actor, previous, message);

        ++concrete_actor->spawned_senders;
        ++concrete_actor->spawned_senders;

    } else if (tag == ACTION_RING_READY && concrete_actor->step == RING_STEP_SPAWN) {
        concrete_actor->ready_rings++;

#if 0
        printf("READY: %d/%d\n", concrete_actor->ready_rings, (int)bsal_vector_size(&concrete_actor->spawners));
#endif

        if (concrete_actor->ready_rings == bsal_vector_size(&concrete_actor->spawners)) {

            thorium_actor_send_range_empty(actor, &concrete_actor->spawners, ACTION_RING_PUSH_NEXT);
            concrete_actor->ready_rings = 0;
            concrete_actor->step = RING_STEP_PUSH_NEXT;
        }
    } else if (tag == ACTION_RING_PUSH_NEXT) {

        previous_actor = bsal_vector_index_of(&concrete_actor->spawners, &name) - 1;
        if (previous_actor < 0) {
            previous_actor = bsal_vector_size(&concrete_actor->spawners)- 1;
        }

        printf("%d received ACTION_RING_PUSH_NEXT\n", name);
        thorium_message_init(message, ACTION_RING_SET_NEXT, sizeof(concrete_actor->first), &concrete_actor->first);
        thorium_actor_send(actor, *(int *)bsal_vector_at(&concrete_actor->spawners, previous_actor),
                       message);

    } else if (tag == ACTION_RING_SET_NEXT) {

        concrete_actor->step = RING_STEP_PUSH_NEXT;
        thorium_message_set_action(message, ACTION_SENDER_SET_NEXT);
        thorium_actor_send(actor, concrete_actor->last, message);

    } else if (tag == ACTION_SENDER_SET_NEXT_REPLY
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
            thorium_actor_send_empty(actor, *(int *)bsal_vector_at(&concrete_actor->spawners, 0), ACTION_RING_READY);
            concrete_actor->ready_senders = 0;

            concrete_actor->last = concrete_actor->previous;
        } else {
            new_actor = thorium_actor_spawn(actor, SCRIPT_SENDER);
            ++concrete_actor->spawned_senders;
            previous = concrete_actor->previous;

            thorium_message_init(message, ACTION_SENDER_SET_NEXT, sizeof(new_actor), &new_actor);
            thorium_actor_send(actor, previous, message);

            concrete_actor->previous = new_actor;
        }
    } else if (tag == ACTION_SENDER_SET_NEXT_REPLY
                    && concrete_actor->step == RING_STEP_PUSH_NEXT) {

        concrete_actor->ready_senders++;

        printf("ACTION_SENDER_SET_NEXT_REPLY %d/%d\n",
                        concrete_actor->ready_senders,
                        1);

        if (concrete_actor->ready_senders == 1) {
            thorium_actor_send_empty(actor, *(int *)bsal_vector_at(&concrete_actor->spawners, 0), ACTION_RING_READY);
            printf("RING_STEP_PUSH_NEXT completed.\n");
            concrete_actor->ready_senders = 0;
        }


    } else if (tag == ACTION_RING_READY && concrete_actor->step == RING_STEP_PUSH_NEXT) {
        concrete_actor->ready_rings++;

        if (concrete_actor->ready_rings == bsal_vector_size(&concrete_actor->spawners)) {

            printf("system is ready...\n");
            messages = 2000007;
            thorium_message_init(message, ACTION_SENDER_HELLO, sizeof(messages), &messages);
            thorium_actor_send(actor, concrete_actor->first, message);

            concrete_actor->ready_rings = 0;
        }
    } else if (tag == ACTION_SENDER_HELLO_REPLY) {

        thorium_actor_send_range_empty(actor, &concrete_actor->spawners, ACTION_RING_KILL);
        thorium_actor_send_empty(actor, concrete_actor->first, ACTION_SENDER_KILL);

    } else if (tag == ACTION_RING_KILL) {

        thorium_actor_send_to_self_empty(actor, ACTION_STOP);
    }
}
