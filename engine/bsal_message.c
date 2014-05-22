
#include "bsal_message.h"

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
