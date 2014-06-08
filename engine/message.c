
#include "message.h"

#include <stdlib.h>
#include <string.h>

void bsal_message_init(struct bsal_message *message, int tag, int count,
                void *buffer)
{
    message->tag = tag;
    message->buffer = buffer;
    message->count = count;

    message->source_actor = -1;
    message->destination_actor = -1;

    /* MPI ranks are set with bsal_node_resolve */
    message->source_node = -1;
    message->destination_node = -1;

    message->routing_source = -1;
    message->routing_destination = -1;
}

void bsal_message_destroy(struct bsal_message *message)
{
    message->source_actor = -1;
    message->destination_actor = -1;
    message->tag = -1;
    message->buffer = NULL;
    message->count= 0;
}

int bsal_message_source(struct bsal_message *message)
{
    return message->source_actor;
}

int bsal_message_destination(struct bsal_message *message)
{
    return message->destination_actor;
}

int bsal_message_source_node(struct bsal_message *message)
{
    return message->source_node;
}

int bsal_message_destination_node(struct bsal_message *message)
{
    return message->destination_node;
}

int bsal_message_tag(struct bsal_message *message)
{
    return message->tag;
}

void bsal_message_set_source(struct bsal_message *message, int source)
{
    message->source_actor = source;
}

void bsal_message_set_destination(struct bsal_message *message, int destination)
{
    message->destination_actor = destination;
}

void bsal_message_print(struct bsal_message *message)
{
}

void *bsal_message_buffer(struct bsal_message *message)
{
    return message->buffer;
}

int bsal_message_count(struct bsal_message *message)
{
    return message->count;
}

void bsal_message_set_source_node(struct bsal_message *message, int source)
{
    message->source_node = source;
}

void bsal_message_set_destination_node(struct bsal_message *message, int destination)
{
    message->destination_node = destination;
}

void bsal_message_set_buffer(struct bsal_message *message, void *buffer)
{
    message->buffer = buffer;
}

void bsal_message_set_tag(struct bsal_message *message, int tag)
{
    message->tag = tag;
}

int bsal_message_metadata_size(struct bsal_message *message)
{
    return sizeof(message->source_actor) + sizeof(message->destination_actor);
}

void bsal_message_write_metadata(struct bsal_message *message)
{
    /* TODO this could be a single memcpy with 2 *sizeof(int)
     * because source_actor and destination_actor are consecutive
     */
    memcpy((char *)message->buffer + message->count, &message->source_actor,
                    sizeof(message->source_actor));

    memcpy((char *)message->buffer + message->count + sizeof(message->source_actor),
            &message->destination_actor, sizeof(message->destination_actor));
}

void bsal_message_read_metadata(struct bsal_message *message)
{
    /* TODO this could be a single memcpy with 2 *sizeof(int)
     * because source_actor and destination_actor are consecutive
     */
    memcpy(&message->source_actor, (char *)message->buffer + message->count,
                    sizeof(message->source_actor));

    memcpy(&message->destination_actor,
            (char *)message->buffer + message->count + sizeof(message->source_actor),
            sizeof(message->destination_actor));
}

void bsal_message_set_count(struct bsal_message *message, int count)
{
    message->count = count;
}

int bsal_message_unpack_int(struct bsal_message *message, int offset, int *value)
{
    int bytes;
    void *buffer;
    int *pointer;

    bytes = sizeof(value);
    buffer = bsal_message_buffer(message);

    pointer = (int *)((char *)buffer + offset);

    *value = *pointer;

    offset += bytes;

    return offset;
}

void bsal_message_get_all(struct bsal_message *message, int *tag, int *count, void **buffer, int *source)
{
    *tag = bsal_message_tag(message);
    *count = bsal_message_count(message);
    *buffer = bsal_message_buffer(message);
    *source = bsal_message_source(message);
}

void bsal_message_init_copy(struct bsal_message *message, struct bsal_message *old_message)
{
    bsal_message_init(message,
                    bsal_message_tag(old_message),
                    bsal_message_count(old_message),
                    bsal_message_buffer(old_message));

    bsal_message_set_source(message,
                    bsal_message_source(old_message));
    bsal_message_set_destination(message,
                    bsal_message_destination(old_message));
}
