
#include <engine/bsal_actor.h>

struct mock_actor {
    int value;
};

void mock_actor_construct(struct mock_actor *actor);
void mock_actor_destruct(struct mock_actor *actor);
void mock_actor_receive(struct bsal_actor *actor, struct bsal_message *message);

struct bsal_actor_vtable mock_actor_vtable;
