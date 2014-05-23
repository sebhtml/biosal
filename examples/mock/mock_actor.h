
#include <engine/bsal_actor.h>

#include "buddy.h"

struct mock_actor {
    struct buddy buddy_actors[3];
    struct bsal_actor bsal_actors[3];
    int value;
};

struct bsal_actor_vtable mock_actor_vtable;

void mock_actor_construct(struct bsal_actor *actor);
void mock_actor_destruct(struct bsal_actor *actor);
void mock_actor_receive(struct bsal_actor *actor, struct bsal_message *message);

void mock_actor_start(struct bsal_actor *actor, struct bsal_message *message);
void mock_actor_spawn_children(struct bsal_actor *actor);
