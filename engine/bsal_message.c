
#include "bsal_message.h"

void bsal_message_construct(struct bsal_message *message, int source, int destination)
{
    message->source_actor = source;
    message->destination_actor = destination;
}

void bsal_message_destruct(struct bsal_message *message)
{
    message->source_actor = -1;
    message->destination_actor = -1;
}

int bsal_message_get_source_actor(struct bsal_message *message)
{
    return message->source_actor;
}

int bsal_message_get_destination_actor(struct bsal_message *message)
{
    return message->destination_actor;
}

int bsal_message_get_source_rank(struct bsal_message *message)
{
    return message->source_rank;
}

int bsal_message_get_destination_rank(struct bsal_message *message)
{
    return message->destination_rank;
}
