
#include <biosal.h>
#include <stdio.h>

#include "mock_actor.h"

void mock_actor_construct(struct mock_actor *actor)
{
    actor->value = 42;
}

void mock_actor_destruct(struct mock_actor *actor)
{
    actor->value = -1;
}

void mock_actor_receive(void *actor, struct bsal_message *message)
{
    struct mock_actor *mock;
    int source;

    mock = (struct mock_actor *)actor;
    source = bsal_message_get_source_actor(message);

    printf("Actor (value: %i) received message from %i\n",
            mock->value, source);
}
