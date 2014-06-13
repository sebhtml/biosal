
#include "actor_helper.h"

#include <structures/vector_iterator.h>
#include <engine/actor.h>
#include <engine/message.h>

#include <stdlib.h>
#include <stdio.h>

void bsal_actor_helper_send_reply_vector(struct bsal_actor *actor, int tag, struct bsal_vector *vector)
{
    int count;
    struct bsal_message message;
    void *buffer;

    count = bsal_vector_pack_size(vector);
    buffer = malloc(count);
    bsal_vector_pack(vector, buffer);

    bsal_message_init(&message, tag, count, buffer);

    bsal_actor_send_reply(actor, &message);

    free(buffer);

    bsal_message_destroy(&message);
}

void bsal_actor_helper_send_reply_empty(struct bsal_actor *actor, int tag)
{
    bsal_actor_helper_send_empty(actor, bsal_actor_source(actor), tag);
}

void bsal_actor_helper_send_to_self_empty(struct bsal_actor *actor, int tag)
{
    bsal_actor_helper_send_empty(actor, bsal_actor_name(actor), tag);
}

void bsal_actor_helper_send_empty(struct bsal_actor *actor, int destination, int tag)
{
    struct bsal_message message;

    bsal_message_init(&message, tag, 0, NULL);
    bsal_actor_send(actor, destination, &message);
}

void bsal_actor_helper_send_to_supervisor_empty(struct bsal_actor *actor, int tag)
{
    bsal_actor_helper_send_empty(actor, bsal_actor_supervisor(actor), tag);
}

void bsal_actor_helper_send_reply_int(struct bsal_actor *actor, int tag, int value)
{
    bsal_actor_helper_send_int(actor, bsal_actor_source(actor), tag, value);
}

void bsal_actor_helper_send_int(struct bsal_actor *actor, int destination, int tag, int value)
{
    struct bsal_message message;

    bsal_message_init(&message, tag, sizeof(value), &value);
    bsal_actor_send(actor, destination, &message);
}

void bsal_actor_helper_get_acquaintances(struct bsal_actor *actor, struct bsal_vector *indices,
                struct bsal_vector *names)
{
    struct bsal_vector_iterator iterator;
    int index;
    int *bucket;
    int name;

    bsal_vector_iterator_init(&iterator, indices);

    while (bsal_vector_iterator_has_next(&iterator)) {
        bsal_vector_iterator_next(&iterator, (void **)&bucket);

        index = *bucket;
        name = bsal_actor_get_acquaintance(actor, index);

        bsal_vector_push_back(names, &name);
    }

    bsal_vector_iterator_destroy(&iterator);
}
