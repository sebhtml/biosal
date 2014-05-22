
#include <stdlib.h>
#include "bsal_actor.h"

void bsal_actor_construct(struct bsal_actor *bsal_actor, void *actor, bsal_receive_fn_t receive)
{
        bsal_actor->actor = actor;
        bsal_actor->receive = receive;
}

void bsal_actor_destruct(struct bsal_actor *bsal_actor)
{
        bsal_actor->actor = NULL;
        bsal_actor->receive = NULL;
}
