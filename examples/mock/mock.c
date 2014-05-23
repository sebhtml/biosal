
#include "mock.h"
#include "buddy.h"

#include <stdio.h>

struct bsal_actor_vtable mock_vtable = {
    .construct = mock_construct,
    .destruct = mock_destruct,
    .receive = mock_receive
};

void mock_construct(struct bsal_actor *actor)
{
    struct mock *mock;
    mock = (struct mock *)bsal_actor_actor(actor);

    mock->value = 42;
}

void mock_destruct(struct bsal_actor *actor)
{
    struct mock *mock;
    mock = (struct mock *)bsal_actor_actor(actor);

    mock->value = -1;
}

void mock_receive(struct bsal_actor *actor, struct bsal_message *message)
{
    int tag;

    tag = bsal_message_tag(message);

    if (tag == BSAL_START) {
        mock_construct(actor);
        mock_start(actor, message);
    }
}

void mock_start(struct bsal_actor *actor, struct bsal_message *message)
{
    struct mock *mock;
    int source;
    int name;
    int destination;
    int tag;

    tag = bsal_message_tag(message);
    source = bsal_message_source(message);
    destination = bsal_message_destination(message);

    name = bsal_actor_name(actor);
    mock = (struct mock *)bsal_actor_actor(actor);

    printf("mock_start Actor #%i (value: %i) received message (tag: %i)"
                    " from source %i, destination %i\n",
            name, mock->value, tag, source, destination);

    mock_spawn_children(actor);

    printf("mock_start actor %i dies\n", name);
    mock_destruct(actor);
    bsal_actor_die(actor);
}

void mock_spawn_children(struct bsal_actor *actor)
{
    int total;
    struct mock *mock;
    struct bsal_message message;
    int i;

    total = 1;

    mock = (struct mock *)bsal_actor_actor(actor);
    bsal_message_construct(&message, BUDDY_HELLO, -1, -1, 0, NULL);

    for (i = 0; i <total; i++) {

        struct buddy *buddy_actor = mock->buddy_actors + i;
        int name;
        int tag;

        name = bsal_actor_spawn(actor, buddy_actor, buddy_receive);
        tag = bsal_message_tag(&message);

        printf("mock_spawn_children sending tag %i to %i\n",
                        tag, name);
        bsal_actor_send(actor, name, &message);
    }

    bsal_message_destruct(&message);
}
