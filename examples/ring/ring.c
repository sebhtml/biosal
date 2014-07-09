
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
    .size = sizeof(struct ring)
};

#define RING_STEP_RECEIVE_SPAWNERS 0
#define RING_STEP_SPAWN 1
#define RING_STEP_PUSH_NEXT 2

void ring_init(struct bsal_actor *actor)
{
    struct ring *ring1;

    ring1 = (struct ring *)bsal_actor_concrete_actor(actor);

    ring1->senders = 100000;

#if 0
    ring1->senders = 100;
#endif
    ring1->first = -1;
    ring1->last = -1;
    ring1->step = RING_STEP_RECEIVE_SPAWNERS;
    ring1->ready_rings = 0;
    ring1->ready_senders = 0;

    bsal_vector_init(&ring1->spawners, sizeof(int));

    bsal_actor_add_script(actor, SENDER_SCRIPT, &sender_script);

    bsal_vector_init(&ring1->spawners, 0);
}

void ring_destroy(struct bsal_actor *actor)
{
    struct ring *ring1;

    ring1 = (struct ring *)bsal_actor_concrete_actor(actor);

    bsal_vector_destroy(&ring1->spawners);
}

void ring_receive(struct bsal_actor *actor, struct bsal_message *message)
{
    int tag;
    int i;
    int new_actor;
    int previous;
    int name;
    struct ring *ring1;
    int messages;
    int previous_actor;
    char *buffer;
    int destination;

    ring1 = (struct ring *)bsal_actor_concrete_actor(actor);
    tag = bsal_message_tag(message);
    buffer = bsal_message_buffer(message);
    name = bsal_actor_name(actor);

    if (tag == BSAL_ACTOR_START) {

        bsal_vector_init(&ring1->spawners, 0);
        bsal_vector_unpack(&ring1->spawners, buffer);
        printf("actor %d BSAL_ACTOR_START, %d spawners\n", name,
                        (int)bsal_vector_size(&ring1->spawners));

        destination = *(int *)bsal_vector_at(&ring1->spawners, 0);
        bsal_actor_helper_send_empty(actor, destination, RING_READY);

    } else if (tag == RING_READY && ring1->step == RING_STEP_RECEIVE_SPAWNERS) {

        ring1->ready_rings++;

        if (ring1->ready_rings == (int)bsal_vector_size(&ring1->spawners)) {
            bsal_actor_helper_send_range_empty(actor, &ring1->spawners, RING_SPAWN);
            ring1->step = RING_STEP_SPAWN;
            ring1->ready_rings = 0;
        }

    } else if (tag == RING_SPAWN) {
        i = 0;

        printf("actor/%d is spawning %d senders\n",
                        bsal_actor_name(actor), ring1->senders);

        ring1->step = RING_STEP_SPAWN;

        previous = -1;
        while (i < ring1->senders) {
            new_actor = bsal_actor_spawn(actor, SENDER_SCRIPT);

            if (i % 10000 == 0) {

                printf("actor/%d spawned %d/%d senders\n",
                        bsal_actor_name(actor), i, ring1->senders);
            }

            if (i > 0) {
                bsal_message_init(message, SENDER_SET_NEXT, sizeof(new_actor), &new_actor);
                bsal_actor_send(actor, previous, message);
            }

            if (i == 0) {
                ring1->first = new_actor;
                printf("actor %d: first is actor %d\n", name, new_actor);
            } else if (i == ring1->senders - 1) {
                ring1->last = new_actor;
                printf("actor %d: last is actor %d\n", name, new_actor);
            }

            previous = new_actor;
            i++;
        }

        printf("actor %d spawned %d actors for a part of the ring\n",
                        name, ring1->senders);

    } else if (tag == RING_READY && ring1->step == RING_STEP_SPAWN) {
        ring1->ready_rings++;

#if 0
        printf("READY: %d/%d\n", ring1->ready_rings, (int)bsal_vector_size(&ring1->spawners));
#endif

        if (ring1->ready_rings == bsal_vector_size(&ring1->spawners)) {

            bsal_actor_helper_send_range_empty(actor, &ring1->spawners, RING_PUSH_NEXT);
            ring1->ready_rings = 0;
            ring1->step = RING_STEP_PUSH_NEXT;
        }
    } else if (tag == RING_PUSH_NEXT) {

        previous_actor = bsal_vector_index_of(&ring1->spawners, &name) - 1;
        if (previous_actor < 0) {
            previous_actor = bsal_vector_size(&ring1->spawners)- 1;
        }

        printf("%d received RING_PUSH_NEXT\n", name);
        bsal_message_init(message, RING_SET_NEXT, sizeof(ring1->first), &ring1->first);
        bsal_actor_send(actor, *(int *)bsal_vector_at(&ring1->spawners, previous_actor),
                       message);

    } else if (tag == RING_SET_NEXT) {

        ring1->step = RING_STEP_PUSH_NEXT;
        bsal_message_set_tag(message, SENDER_SET_NEXT);
        bsal_actor_send(actor, ring1->last, message);

    } else if (tag == SENDER_SET_NEXT_REPLY) {

        ring1->ready_senders++;

#if 0
        printf("ready senders %d/%d\n", ring1->ready_senders, ring1->senders);
#endif

        if (ring1->step == RING_STEP_SPAWN && ring1->ready_senders == ring1->senders - 1) {

            printf("RING_STEP_SPAWN completed.\n");
            bsal_actor_helper_send_empty(actor, *(int *)bsal_vector_at(&ring1->spawners, 0), RING_READY);
            ring1->ready_senders = 0;

        } else if (ring1->step == RING_STEP_PUSH_NEXT && ring1->ready_senders == 1) {
            bsal_actor_helper_send_empty(actor, *(int *)bsal_vector_at(&ring1->spawners, 0), RING_READY);
            printf("RING_STEP_PUSH_NEXT completed.\n");
            ring1->ready_senders = 0;
        }



    } else if (tag == RING_READY && ring1->step == RING_STEP_PUSH_NEXT) {
        ring1->ready_rings++;

        if (ring1->ready_rings == bsal_vector_size(&ring1->spawners)) {

            printf("system is ready...\n");
            messages = 2000007;
            bsal_message_init(message, SENDER_HELLO, sizeof(messages), &messages);
            bsal_actor_send(actor, ring1->first, message);

            ring1->ready_rings = 0;
        }
    } else if (tag == SENDER_HELLO_REPLY) {

        bsal_actor_helper_send_range_empty(actor, &ring1->spawners, RING_KILL);
        bsal_actor_helper_send_empty(actor, ring1->first, SENDER_KILL);

    } else if (tag == RING_KILL) {

        bsal_actor_helper_send_to_self_empty(actor, BSAL_ACTOR_STOP);
    }
}
