
#ifndef _MOCK_H
#define _MOCK_H

#include "buddy.h"

struct mock {
    struct buddy buddy_actors[3];
    int value;
};

enum {
    MOCK_DIE = 1200 /* FIRST_TAG */ /* LAST_TAG */
};

struct bsal_actor_vtable mock_vtable;

void mock_construct(struct bsal_actor *actor);
void mock_destruct(struct bsal_actor *actor);
void mock_receive(struct bsal_actor *actor, struct bsal_message *message);

void mock_start(struct bsal_actor *actor, struct bsal_message *message);
void mock_spawn_children(struct bsal_actor *actor);
void mock_die(struct bsal_actor *actor, struct bsal_message *message);

#endif
