
#include "helper.h"

#include <engine/message.h>

#include <stdlib.h>

void bsal_helper_init(struct bsal_helper *self)
{
    self->mock = 0;
}

void bsal_helper_destroy(struct bsal_helper *self)
{
    self->mock = 0;
}

void bsal_helper_send_reply_vector(struct bsal_actor *actor, int tag, struct bsal_vector *vector)
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

void bsal_helper_send_reply_empty(struct bsal_actor *actor, int tag)
{
    bsal_helper_send_empty(actor, bsal_actor_source(actor), tag);
}

void bsal_helper_send_to_self_empty(struct bsal_actor *actor, int tag)
{
    bsal_helper_send_empty(actor, bsal_actor_name(actor), tag);
}

void bsal_helper_send_empty(struct bsal_actor *actor, int destination, int tag)
{
    struct bsal_message message;

    bsal_message_init(&message, tag, 0, NULL);
    bsal_actor_send(actor, destination, &message);
}

void bsal_helper_send_to_supervisor_empty(struct bsal_actor *actor, int tag)
{
    bsal_helper_send_empty(actor, bsal_actor_supervisor(actor), tag);
}

void bsal_helper_send_reply_int(struct bsal_actor *actor, int tag, int value)
{
    bsal_helper_send_int(actor, bsal_actor_source(actor), tag, value);
}

void bsal_helper_send_int(struct bsal_actor *actor, int destination, int tag, int value)
{
    struct bsal_message message;

    bsal_message_init(&message, tag, sizeof(value), &value);
    bsal_actor_send(actor, destination, &message);
}

int bsal_helper_unpack_int(struct bsal_message *message, int offset, int *value)
{
    int bytes;
    void *buffer;
    int *pointer;

    if (offset >= bsal_message_count(message)) {
        return -1;
    }

    bytes = sizeof(value);
    buffer = bsal_message_buffer(message);

    pointer = (int *)((char *)buffer + offset);

    *value = *pointer;

    offset += bytes;

    return offset;
}

void bsal_helper_get_all(struct bsal_message *message, int *tag, int *count, void **buffer, int *source)
{
    *tag = bsal_message_tag(message);
    *count = bsal_message_count(message);
    *buffer = bsal_message_buffer(message);
    *source = bsal_message_source(message);
}


