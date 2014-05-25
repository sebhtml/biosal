
#include "actor_vtable.h"

#include <stdlib.h>

void bsal_actor_vtable_init(struct bsal_actor_vtable *vtable, bsal_actor_init_fn_t init,
                bsal_actor_destroy_fn_t destroy, bsal_actor_receive_fn_t receive)
{
    vtable->init = init;
    vtable->destroy = destroy;
    vtable->receive = receive;
}

void bsal_actor_vtable_destroy(struct bsal_actor_vtable *vtable)
{
    vtable->init = NULL;
    vtable->destroy = NULL;
    vtable->receive = NULL;
}

bsal_actor_init_fn_t bsal_actor_vtable_get_init(struct bsal_actor_vtable *vtable)
{
    return vtable->init;
}

bsal_actor_destroy_fn_t bsal_actor_vtable_get_destroy(struct bsal_actor_vtable *vtable)
{
    return vtable->destroy;
}

bsal_actor_receive_fn_t bsal_actor_vtable_get_receive(struct bsal_actor_vtable *vtable)
{
    return vtable->receive;
}
