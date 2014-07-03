
#include "script.h"

#include <stdlib.h>

void bsal_script_init(struct bsal_script *script, bsal_actor_init_fn_t init,
                bsal_actor_destroy_fn_t destroy, bsal_actor_receive_fn_t receive)
{
    script->init = init;
    script->destroy = destroy;
    script->receive = receive;
}

void bsal_script_destroy(struct bsal_script *script)
{
    script->init = NULL;
    script->destroy = NULL;
    script->receive = NULL;
}

bsal_actor_init_fn_t bsal_script_get_init(struct bsal_script *script)
{
    return script->init;
}

bsal_actor_destroy_fn_t bsal_script_get_destroy(struct bsal_script *script)
{
    return script->destroy;
}

bsal_actor_receive_fn_t bsal_script_get_receive(struct bsal_script *script)
{
    return script->receive;
}

int bsal_script_name(struct bsal_script *script)
{
    return script->name;
}

int bsal_script_size(struct bsal_script *script)
{
    return script->size;
}

char *bsal_script_description(struct bsal_script *self)
{
    return self->description;
}
