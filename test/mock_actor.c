
#include <biosal.h>

#include "mock_actor.h"

void mock_actor_construct(struct mock_actor *actor)
{
    actor->value = 42;
}

void mock_actor_destruct(struct mock_actor *actor)
{
    actor->value = -1;
}

void mock_actor_receive(struct bsal_actor *actor, struct bsal_message *message)
{

   struct mock_actor *mock = (struct mock_actor *)actor;

   int source = bsal_message_get_source_actor(message);

}
