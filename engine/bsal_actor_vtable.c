
#include "bsal_actor_vtable.h"

#include <stdlib.h>

bsal_receive_fn_t bsal_actor_vtable_receive(struct bsal_actor_vtable *vtable)
{
    return vtable->receive;
}

void bsal_actor_vtable_construct(struct bsal_actor_vtable *vtable, bsal_receive_fn_t receive)
{
    vtable->receive = receive;
}

void bsal_actor_vtable_destruct(struct bsal_actor_vtable *vtable)
{
    vtable->receive = NULL;
}
