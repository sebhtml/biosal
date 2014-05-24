
#ifndef _BUDDY_H
#define _BUDDY_H

#include <engine/bsal_actor.h>

struct buddy {
    int received;
};

enum {
    BUDDY_DIE = 1100 /* FIRST_TAG */ /* LAST_TAG */
};

struct bsal_actor_vtable buddy_vtable;

void buddy_construct(struct bsal_actor *actor);
void buddy_destruct(struct bsal_actor *actor);
void buddy_receive(struct bsal_actor *actor, struct bsal_message *message);

#endif
