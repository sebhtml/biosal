
#include "mock_actor.h"
#include "buddy.h"

#include <stdio.h>

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

    printf("mock_actor_start Actor #%i (value: %i) received message (tag: %i)"
                    " from source %i, destination %i\n",
            name, mock->value, tag, source, destination);

    mock_actor_spawn_children(actor);
}

void mock_actor_spawn_children(struct bsal_actor *actor)
{
    int total;
    struct mock_actor *mock;
    struct bsal_message message;
    int i;

    total = 1;

    mock = (struct mock_actor *)bsal_actor_actor(actor);
    bsal_message_construct(&message, BUDDY_HELLO, -1, -1, 0, NULL);

    for (i = 0; i <total; i++) {

        struct buddy *buddy_actor = mock->buddy_actors + i;
        struct bsal_actor *bsal_actor = mock->bsal_actors + i;
        int name;
        int tag;

        bsal_actor_construct(bsal_actor, buddy_actor, &buddy_vtable);

        name = bsal_actor_spawn(actor, bsal_actor);
        tag = bsal_message_tag(&message);

        printf("mock_actor_spawn_children sending tag %i to %i\n",
                        tag, name);
        bsal_actor_send(actor, name, &message);
    }

    bsal_message_destruct(&message);
}
