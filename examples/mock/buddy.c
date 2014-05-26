
#include "buddy.h"

#include <stdio.h>

/* this vtable is required */
struct bsal_actor_vtable buddy_vtable = {
    .init = buddy_init,
    .destroy = buddy_destroy,
    .receive = buddy_receive
};

void buddy_init(struct bsal_actor *actor)
{
    struct buddy *buddy1;

    buddy1 = (struct buddy *)bsal_actor_actor(actor);
    buddy1->received = 0;
}

void buddy_destroy(struct bsal_actor *actor)
{
    struct buddy *buddy1;

    buddy1 = (struct buddy *)bsal_actor_actor(actor);
    buddy1->received = -1;
}

void buddy_receive(struct bsal_actor *actor, struct bsal_message *message)
{
    int tag;
    int source;
    int name;

    name = bsal_actor_name(actor);
    source = bsal_message_source(message);
    tag = bsal_message_tag(message);

    if (tag == BUDDY_DIE) {
        bsal_actor_print(actor);
        printf("buddy_receive Actor %i received a message (%i BUDDY_DIE) from actor %i\n",
                        name, tag, source);

        bsal_actor_die(actor);
    }
}
