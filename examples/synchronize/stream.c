
#include "stream.h"

#include <stdio.h>

struct thorium_script stream_script = {
    .identifier = SCRIPT_STREAM,
    .init = stream_init,
    .destroy = stream_destroy,
    .receive = stream_receive,
    .size = sizeof(struct stream),
    .name = "stream"
};

void stream_init(struct thorium_actor *actor)
{
    struct stream *stream1;

    stream1 = (struct stream *)thorium_actor_concrete_actor(actor);
    stream1->initial_synchronization = 1;
    stream1->ready = 0;

    biosal_vector_init(&stream1->children, sizeof(int));
    biosal_vector_init(&stream1->spawners, sizeof(int));
    stream1->is_king = 0;

    thorium_actor_add_script(actor, SCRIPT_STREAM, &stream_script);
}

void stream_destroy(struct thorium_actor *actor)
{
    struct stream *stream1;

    stream1 = (struct stream *)thorium_actor_concrete_actor(actor);

    biosal_vector_destroy(&stream1->children);
    biosal_vector_destroy(&stream1->spawners);
}

void stream_receive(struct thorium_actor *actor, struct thorium_message *message)
{
    int tag;
    int to_spawn;
    int i;
    int name;
    int king;
    struct stream *stream1;
    int new_actor;
    char *buffer;

    stream1 = (struct stream *)thorium_actor_concrete_actor(actor);
    tag = thorium_message_action(message);
    buffer = thorium_message_buffer(message);
    name = thorium_actor_name(actor);

    to_spawn = 30000;

    if (tag == ACTION_START) {

        biosal_vector_init(&stream1->spawners, 0);
        biosal_vector_unpack(&stream1->spawners, buffer);
        king = *(int *)biosal_vector_at(&stream1->spawners, biosal_vector_size(&stream1->spawners) / 2);

        printf("actor %d says that king is %d\n",
                        thorium_actor_name(actor), king);

        if (name == king) {
            stream1->is_king = 1;
        }

        i = 0;

        while (i < to_spawn) {
            new_actor = thorium_actor_spawn(actor, SCRIPT_STREAM);

            biosal_vector_push_back(&stream1->children, &new_actor);
            i++;
        }

        printf("actor %d spawned %d actors\n", name, to_spawn);

        /* synchronize with other initial actors
         */

        if (!stream1->is_king) {
            return;
        }

        thorium_actor_synchronize(actor, &stream1->spawners);

    } else if (tag == ACTION_SYNCHRONIZED && stream1->initial_synchronization == 1) {

        printf("king %d synchronized\n", name);

        stream1->ready = 0;
        stream1->initial_synchronization = 0;

        thorium_actor_send_range_empty(actor, &stream1->spawners, ACTION_STREAM_SYNC);

    } else if (tag == ACTION_SYNCHRONIZE) {

        thorium_actor_send_reply_empty(actor, ACTION_SYNCHRONIZE_REPLY);

    } else if (tag == ACTION_SYNCHRONIZE_REPLY && stream1->initial_synchronization == 0) {

        stream1->ready++;

        if (stream1->ready % 1000 == 0) {
            printf("ACTION_SYNCHRONIZE_REPLY %d/%d\n", stream1->ready,
                        (int)biosal_vector_size(&stream1->children));
        }

        if (stream1->ready == biosal_vector_size(&stream1->children)) {

            printf("READY\n");
            thorium_actor_send_range_empty(actor, &stream1->spawners, ACTION_STREAM_DIE);
        }

    } else if (tag == ACTION_STREAM_SYNC) {

        stream1->ready = 0;

        /* synchronize with everyone ! */

        printf("actor %d sends ACTION_SYNCHRONIZE manually to %d actors\n",
                        name, (int)biosal_vector_size(&stream1->children));

        for (i = 0 ; i < biosal_vector_size(&stream1->children); i++) {

            if (i % 1000 == 0) {
                printf("sending %i/%i\n", i, (int)biosal_vector_size(&stream1->children));
            }
            new_actor = *(int *)biosal_vector_at(&stream1->children, i);

            thorium_actor_send_empty(actor, new_actor, ACTION_SYNCHRONIZE);
        }

        printf("done...\n");
    } else if (tag == ACTION_STREAM_DIE) {

        for (i = 0 ; i < biosal_vector_size(&stream1->children); i++) {
            new_actor = *(int *)biosal_vector_at(&stream1->children, i);

            thorium_actor_send_empty(actor, new_actor, ACTION_STREAM_DIE);
        }

        thorium_actor_send_to_self_empty(actor, ACTION_STOP);
    }
}

