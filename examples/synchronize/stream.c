
#include "stream.h"

#include <stdio.h>

struct bsal_script stream_script = {
    .name = STREAM_SCRIPT,
    .init = stream_init,
    .destroy = stream_destroy,
    .receive = stream_receive,
    .size = sizeof(struct stream)
};

void stream_init(struct bsal_actor *actor)
{
    struct stream *stream1;

    stream1 = (struct stream *)bsal_actor_concrete_actor(actor);
    stream1->initial_synchronization = 0;
}

void stream_destroy(struct bsal_actor *actor)
{
}

void stream_receive(struct bsal_actor *actor, struct bsal_message *message)
{
    int tag;
    int source;
    int nodes;
    int to_spawn;
    int i;
    int name;
    int king;
    int is_king;
    struct stream *stream1;
    int synchronized_actors;
    void *buffer;

    stream1 = (struct stream *)bsal_actor_concrete_actor(actor);
    tag = bsal_message_tag(message);
    nodes = bsal_actor_nodes(actor);
    name = bsal_actor_name(actor);

    king = nodes / 2;

    is_king = 0;

    if (name == king) {
        is_king = 1;
    }

    to_spawn = 250000;

    if (tag == BSAL_ACTOR_START) {

        i = 0;

        while (i < to_spawn) {
            bsal_actor_spawn(actor, STREAM_SCRIPT);
            i++;
        }

        printf("actor %d spawned %d actors\n", name, to_spawn);

        /* synchronize with other initial actors
         */

        if (!is_king) {
            return;
        }

        bsal_actor_synchronize(actor, 0, nodes - 1);

    } else if (tag == BSAL_ACTOR_SYNCHRONIZE) {
        source = bsal_message_source(message);

        bsal_message_init(message, BSAL_ACTOR_SYNCHRONIZE_REPLY, 0, NULL);
        bsal_actor_send(actor, source, message);

    } else if (tag == BSAL_ACTOR_SYNCHRONIZED) {

        if (!stream1->initial_synchronization) {
            stream1->initial_synchronization = 1;

            printf("actor %d synchronized with initial %d actors\n", name, nodes);

            /* synchronize with everyone ! */
            bsal_actor_synchronize(actor, 0, to_spawn * nodes + nodes - 1);

        } else {

            buffer = bsal_message_buffer(message);
            synchronized_actors = *(int *)buffer;

            printf("actor %d synchronized with %d actors\n", name,
                            synchronized_actors);

            bsal_message_init(message, STREAM_DIE, 0, NULL);
            bsal_actor_send_range_standard(actor, 0, synchronized_actors - 1, message);
        }
    } else if (tag == STREAM_DIE) {
        bsal_message_init(message, BSAL_ACTOR_STOP, 0, NULL);
        bsal_actor_send(actor, name, message);
    }
}

