
#include "bsal_actor.h"

#include <stdlib.h>
#include <stdio.h>


void bsal_actor_construct(struct bsal_actor *bsal_actor, void *actor, bsal_receive_fn_t receive)
{
    bsal_actor->actor = actor;
    bsal_actor->name = -1;

    bsal_actor->receive = receive;

    /*
    TODO: use a vtable... (where do I allocate memory for  the vtable ?)
    struct bsal_actor_vtable *vtable = ...;
    bsal_actor_vtable_construct(vtable,  receive);
    bsal_actor->vtable = vtable;
    */
    /* printf("bsal_actor_construct %p %p %p\n", (void*)bsal_actor, (void*)actor, (void*)receive); */
}

void bsal_actor_destruct(struct bsal_actor *bsal_actor)
{
    bsal_actor->actor = NULL;
    bsal_actor->receive = NULL;
}

int bsal_actor_name(struct bsal_actor *bsal_actor)
{
    return bsal_actor->name;
}

void *bsal_actor_actor(struct bsal_actor *bsal_actor)
{
    return bsal_actor->actor;
}

bsal_receive_fn_t bsal_actor_handler(struct bsal_actor *bsal_actor)
{
    /* printf("bsal_actor_handler %p %p\n", (void*)bsal_actor, (void*)bsal_actor->receive); */

    return bsal_actor->receive;
    /* return bsal_actor_vtable_receive(bsal_actor->vtable); */
}

void bsal_actor_set_name(struct bsal_actor *actor, int name)
{
    actor->name = name;
}

void bsal_actor_print(struct bsal_actor *actor)
{
    printf("bsal_actor_print name: %i %p %p %p\n", bsal_actor_name(actor),
                    (void*)actor, (void*)bsal_actor_actor(actor),
                    (void*)bsal_actor_handler(actor));
}
