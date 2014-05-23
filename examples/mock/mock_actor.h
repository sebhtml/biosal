
#include <engine/bsal_actor.h>

struct mock_actor {
    int value;
};

struct bsal_actor_vtable mock_actor_vtable;

void mock_actor_construct(struct bsal_actor *actor);
void mock_actor_destruct(struct bsal_actor *actor);
void mock_actor_receive(struct bsal_actor *actor, struct bsal_message *message);

void mock_actor_start(struct bsal_actor *actor, struct bsal_message *message);
