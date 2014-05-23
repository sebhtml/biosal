
#include "bsal_actor_vtable.h"

#include <stdlib.h>

void bsal_actor_vtable_construct(struct bsal_actor_vtable *vtable, bsal_actor_construct_fn_t construct,
                bsal_actor_destruct_fn_t destruct, bsal_actor_receive_fn_t receive)
{
    vtable->construct = construct;
    vtable->destruct = destruct;
    vtable->receive = receive;
}

void bsal_actor_vtable_destruct(struct bsal_actor_vtable *vtable)
{
    vtable->receive = NULL;
}

bsal_actor_construct_fn_t bsal_actor_vtable_get_construct(struct bsal_actor_vtable *vtable)
{
    return vtable->construct;
}

bsal_actor_destruct_fn_t bsal_actor_vtable_get_destruct(struct bsal_actor_vtable *vtable)
{
    return vtable->destruct;
}

bsal_actor_receive_fn_t bsal_actor_vtable_get_receive(struct bsal_actor_vtable *vtable)
{
    return vtable->receive;
}
