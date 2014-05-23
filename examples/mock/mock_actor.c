
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
    int tag;

    tag = bsal_message_tag(message);

    if (tag == BSAL_START) {
        mock_actor_start(actor, message);
    }
}

void mock_actor_start(struct bsal_actor *actor, struct bsal_message *message)
{
    struct mock_actor *mock;
    int source;
    int name;
    int destination;
    int tag;

    tag = bsal_message_tag(message);
    source = bsal_message_source(message);
    destination = bsal_message_destination(message);

    name = bsal_actor_name(actor);
    mock = (struct mock_actor *)bsal_actor_actor(actor);

    printf("Actor #%i (value: %i) received message (tag: %i)"
                    " from source %i, destination %i\n",
            name, mock->value, tag, source, destination);

}
