
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

void ring_init(struct bsal_actor *actor)
{
    struct ring *ring1;

    ring1 = (struct ring *)bsal_actor_concrete_actor(actor);

    ring1->senders = 100000;
    ring1->first = -1;
    ring1->last = -1;
    ring1->step = 0;
    ring1->ready = 0;
}

void ring_destroy(struct bsal_actor *actor)
{
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
    int size;

    ring1 = (struct ring *)bsal_actor_concrete_actor(actor);
    tag = bsal_message_tag(message);
    name = bsal_actor_name(actor);
    size = bsal_actor_nodes(actor);

    if (tag == BSAL_ACTOR_START) {

        printf("actor %d BSAL_ACTOR_START\n", name);

        bsal_actor_add_script(actor, SENDER_SCRIPT, &sender_script);

        i = 0;
        while (i < ring1->senders) {
            new_actor = bsal_actor_spawn(actor, SENDER_SCRIPT);

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

        bsal_actor_send_empty(actor, 0, RING_READY);

    } else if (tag == RING_READY && ring1->step == 0) {
        ring1->ready++;

        if (ring1->ready == size) {

            bsal_actor_send_range_standard_empty(actor, 0, size - 1, RING_PUSH_NEXT);
            ring1->step++;
            ring1->ready = 0;
        }
    } else if (tag == RING_PUSH_NEXT) {

        previous_actor = name - 1;
        if (previous_actor < 0) {
            previous_actor = size - 1;
        }

        bsal_message_init(message, RING_SET_NEXT, sizeof(ring1->first), &ring1->first);
        bsal_actor_send(actor, previous_actor, message);

    } else if (tag == RING_SET_NEXT) {

        bsal_message_set_tag(message, SENDER_SET_NEXT);
        bsal_actor_send(actor, ring1->last, message);

    } else if (tag == SENDER_SET_NEXT_REPLY) {

        bsal_actor_send_empty(actor, 0, RING_READY);

    } else if (tag == RING_READY && ring1->step == 1) {
        ring1->ready++;

        if (ring1->ready == size) {

            messages = 2000007;
            bsal_message_init(message, SENDER_HELLO, sizeof(messages), &messages);
            bsal_actor_send(actor, ring1->first, message);
        }
    } else if (tag == SENDER_HELLO_REPLY) {

        bsal_actor_send_range_standard_empty(actor, 0, size - 1, RING_KILL);
        bsal_actor_send_empty(actor, ring1->first, SENDER_KILL);

    } else if (tag == RING_KILL) {

        bsal_actor_send_to_self_empty(actor, BSAL_ACTOR_STOP);
    }
}
