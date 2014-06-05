
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
    stream1->initial_synchronization = 1;
    stream1->ready = 0;

    bsal_vector_init(&stream1->children, sizeof(int));
}

void stream_destroy(struct bsal_actor *actor)
{
    struct stream *stream1;

    stream1 = (struct stream *)bsal_actor_concrete_actor(actor);

    bsal_vector_destroy(&stream1->children);
}

void stream_receive(struct bsal_actor *actor, struct bsal_message *message)
{
    int tag;
    int nodes;
    int to_spawn;
    int i;
    int name;
    int king;
    int is_king;
    struct stream *stream1;
    int new_actor;

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

        bsal_actor_add_script(actor, STREAM_SCRIPT, &stream_script);

        i = 0;

        while (i < to_spawn) {
            new_actor = bsal_actor_spawn(actor, STREAM_SCRIPT);

            bsal_vector_push_back(&stream1->children, &new_actor);
            i++;
        }

        printf("actor %d spawned %d actors\n", name, to_spawn);

        /* synchronize with other initial actors
         */

        if (!is_king) {
            return;
        }

        bsal_actor_synchronize(actor, 0, nodes - 1);

    } else if (tag == BSAL_ACTOR_SYNCHRONIZED && stream1->initial_synchronization == 1) {

        printf("king %d synchronized\n", name);

        stream1->ready = 0;
        stream1->initial_synchronization = 0;

        bsal_actor_send_range_standard_empty(actor, 0, nodes - 1, STREAM_SYNC);

    } else if (tag == BSAL_ACTOR_SYNCHRONIZE) {

        bsal_actor_send_reply_empty(actor, BSAL_ACTOR_SYNCHRONIZE_REPLY);

    } else if (tag == BSAL_ACTOR_SYNCHRONIZE_REPLY && stream1->initial_synchronization == 0) {

        stream1->ready++;

        if (stream1->ready == bsal_vector_size(&stream1->children)) {

            bsal_actor_send_range_standard_empty(actor, 0, nodes - 1, STREAM_DIE);
        }

    } else if (tag == STREAM_SYNC) {

        stream1->ready = 0;

        /* synchronize with everyone ! */

        printf("actor %d sends BSAL_ACTOR_SYNCHRONIZE manually to %d actors\n",
                        name, (int)bsal_vector_size(&stream1->children));

        for (i = 0 ; i < bsal_vector_size(&stream1->children); i++) {
            new_actor = *(int *)bsal_vector_at(&stream1->children, i);

            bsal_actor_send_empty(actor, new_actor, BSAL_ACTOR_SYNCHRONIZE);
        }

    } else if (tag == STREAM_DIE) {

        for (i = 0 ; i < bsal_vector_size(&stream1->children); i++) {
            new_actor = *(int *)bsal_vector_at(&stream1->children, i);

            bsal_actor_send_empty(actor, new_actor, STREAM_DIE);
        }

        bsal_actor_send_to_self_empty(actor, BSAL_ACTOR_STOP);
    }
}

