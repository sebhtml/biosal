
#include "buddy.h"

#include <stdio.h>

/* this vtable is not required anymore */
struct bsal_actor_vtable buddy_vtable = {
    .construct = buddy_construct,
    .destruct = buddy_destruct,
    .receive = buddy_receive
};

void buddy_construct(struct bsal_actor *actor)
{
    ((struct buddy *)(bsal_actor_actor(actor)))->received = 0;
}

void buddy_destruct(struct bsal_actor *actor)
{
    ((struct buddy *)(bsal_actor_actor(actor)))->received = -1;
}

void buddy_receive(struct bsal_actor *actor, struct bsal_message *message)
{
    int tag;
    int source;
    int name;

    name = bsal_actor_name(actor);
    source = bsal_message_source(message);
    tag = bsal_message_tag(message);

    if (tag == BUDDY_HELLO) {
        buddy_construct(actor);
        printf("buddy_receive Actor %i received a message (%i) from actor %i\n",
                        name, tag, source);
    }
}
