
#include "message.h"

#include <stdlib.h>
#include <string.h>

void bsal_message_init(struct bsal_message *message, int tag, int source,
                int destination, int count, char *buffer)
{
    message->source_actor = source;
    message->destination_actor = destination;

    message->tag = tag;
    message->buffer = buffer;
    message->count = count;

    /* MPI ranks are set with bsal_node_resolve */
    message->source_rank = -1;
    message->destination_rank = -1;

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

int bsal_message_source_rank(struct bsal_message *message)
{
    return message->source_rank;
}

int bsal_message_destination_rank(struct bsal_message *message)
{
    return message->destination_rank;
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

char *bsal_message_buffer(struct bsal_message *message)
{
    return message->buffer;
}

int bsal_message_count(struct bsal_message *message)
{
    return message->count;
}

void bsal_message_set_source_rank(struct bsal_message *message, int source)
{
    message->source_rank = source;
}

void bsal_message_set_destination_rank(struct bsal_message *message, int destination)
{
    message->destination_rank = destination;
}

void bsal_message_set_buffer(struct bsal_message *message, char *buffer)
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
    memcpy(message->buffer + message->count, &message->source_actor,
                    sizeof(message->source_actor));

    memcpy(message->buffer + message->count + sizeof(message->source_actor),
            &message->destination_actor, sizeof(message->destination_actor));
}

void bsal_message_read_metadata(struct bsal_message *message)
{
    /* TODO this could be a single memcpy with 2 *sizeof(int)
     * because source_actor and destination_actor are consecutive
     */
    memcpy(&message->source_actor, message->buffer + message->count,
                    sizeof(message->source_actor));

    memcpy(&message->destination_actor,
            message->buffer + message->count + sizeof(message->source_actor),
            sizeof(message->destination_actor));
}
