
#include "stream.h"

#include <stdio.h>

struct bsal_script stream_script = {
    .identifier = STREAM_SCRIPT,
    .init = stream_init,
    .destroy = stream_destroy,
    .receive = stream_receive,
    .size = sizeof(struct stream),
    .name = "stream"
};

void stream_init(struct bsal_actor *actor)
{
    struct stream *stream1;

    stream1 = (struct stream *)bsal_actor_concrete_actor(actor);
    stream1->initial_synchronization = 1;
    stream1->ready = 0;

    bsal_vector_init(&stream1->children, sizeof(int));
    bsal_vector_init(&stream1->spawners, sizeof(int));
    stream1->is_king = 0;

    bsal_actor_add_script(actor, STREAM_SCRIPT, &stream_script);
}

void stream_destroy(struct bsal_actor *actor)
{
    struct stream *stream1;

    stream1 = (struct stream *)bsal_actor_concrete_actor(actor);

    bsal_vector_destroy(&stream1->children);
    bsal_vector_destroy(&stream1->spawners);
}

void stream_receive(struct bsal_actor *actor, struct bsal_message *message)
{
    int tag;
    int to_spawn;
    int i;
    int name;
    int king;
    struct stream *stream1;
    int new_actor;
    char *buffer;

    stream1 = (struct stream *)bsal_actor_concrete_actor(actor);
    tag = bsal_message_tag(message);
    buffer = bsal_message_buffer(message);
    name = bsal_actor_name(actor);

    to_spawn = 30000;

    if (tag == BSAL_ACTOR_START) {

        bsal_vector_init(&stream1->spawners, 0);
        bsal_vector_unpack(&stream1->spawners, buffer);
        king = *(int *)bsal_vector_at(&stream1->spawners, bsal_vector_size(&stream1->spawners) / 2);

        printf("actor %d says that king is %d\n",
                        bsal_actor_name(actor), king);

        if (name == king) {
            stream1->is_king = 1;
        }

        i = 0;

        while (i < to_spawn) {
            new_actor = bsal_actor_spawn(actor, STREAM_SCRIPT);

            bsal_vector_push_back(&stream1->children, &new_actor);
            i++;
        }

        printf("actor %d spawned %d actors\n", name, to_spawn);

        /* synchronize with other initial actors
         */

        if (!stream1->is_king) {
            return;
        }

        bsal_actor_synchronize(actor, &stream1->spawners);

    } else if (tag == BSAL_ACTOR_SYNCHRONIZED && stream1->initial_synchronization == 1) {

        printf("king %d synchronized\n", name);

        stream1->ready = 0;
        stream1->initial_synchronization = 0;

        bsal_actor_send_range_empty(actor, &stream1->spawners, STREAM_SYNC);

    } else if (tag == BSAL_ACTOR_SYNCHRONIZE) {

        bsal_actor_send_reply_empty(actor, BSAL_ACTOR_SYNCHRONIZE_REPLY);

    } else if (tag == BSAL_ACTOR_SYNCHRONIZE_REPLY && stream1->initial_synchronization == 0) {

        stream1->ready++;

        if (stream1->ready % 1000 == 0) {
            printf("BSAL_ACTOR_SYNCHRONIZE_REPLY %d/%d\n", stream1->ready,
                        (int)bsal_vector_size(&stream1->children));
        }

        if (stream1->ready == bsal_vector_size(&stream1->children)) {

            printf("READY\n");
            bsal_actor_send_range_empty(actor, &stream1->spawners, STREAM_DIE);
        }

    } else if (tag == STREAM_SYNC) {

        stream1->ready = 0;

        /* synchronize with everyone ! */

        printf("actor %d sends BSAL_ACTOR_SYNCHRONIZE manually to %d actors\n",
                        name, (int)bsal_vector_size(&stream1->children));

        for (i = 0 ; i < bsal_vector_size(&stream1->children); i++) {

            if (i % 1000 == 0) {
                printf("sending %i/%i\n", i, (int)bsal_vector_size(&stream1->children));
            }
            new_actor = *(int *)bsal_vector_at(&stream1->children, i);

            bsal_actor_send_empty(actor, new_actor, BSAL_ACTOR_SYNCHRONIZE);
        }

        printf("done...\n");
    } else if (tag == STREAM_DIE) {

        for (i = 0 ; i < bsal_vector_size(&stream1->children); i++) {
            new_actor = *(int *)bsal_vector_at(&stream1->children, i);

            bsal_actor_send_empty(actor, new_actor, STREAM_DIE);
        }

        bsal_actor_send_to_self_empty(actor, BSAL_ACTOR_STOP);
    }
}

