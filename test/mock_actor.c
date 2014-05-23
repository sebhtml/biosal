
#include <biosal.h>
#include <stdio.h>

#include "mock_actor.h"

struct bsal_actor_vtable mock_actor_vtable = {
    .construct = mock_actor_construct,
    .destruct = mock_actor_destruct,
    .receive = mock_actor_receive
};

void mock_actor_construct(struct bsal_actor *actor)
{
    struct mock_actor *mock;
    mock = (struct mock_actor *)bsal_actor_actor(actor);

    mock->value = 42;
}

void mock_actor_destruct(struct bsal_actor *actor)
{
    struct mock_actor *mock;
    mock = (struct mock_actor *)bsal_actor_actor(actor);

    mock->value = -1;
}

void mock_actor_receive(struct bsal_actor *actor, struct bsal_message *message)
{
    struct mock_actor *mock;
    int source;
    int name;
    int destination;

    mock = (struct mock_actor *)bsal_actor_actor(actor);
    name = bsal_actor_name(actor);
    source = bsal_message_get_source_actor(message);
    destination = bsal_message_get_destination_actor(message);

    printf("Actor #%i (value: %i) received message from source %i, destination %i\n",
            name, mock->value, source, destination);
}
