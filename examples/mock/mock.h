
#include "buddy.h"

struct mock {
    struct buddy buddy_actors[3];
    int value;
};

struct bsal_actor_vtable mock_vtable;

void mock_construct(struct bsal_actor *actor);
void mock_destruct(struct bsal_actor *actor);
void mock_receive(struct bsal_actor *actor, struct bsal_message *message);

void mock_start(struct bsal_actor *actor, struct bsal_message *message);
void mock_spawn_children(struct bsal_actor *actor);
