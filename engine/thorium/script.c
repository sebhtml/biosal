
#include "script.h"

#include <stdlib.h>

void bsal_script_init(struct bsal_script *self, int identifier,
                bsal_actor_init_fn_t init,
                bsal_actor_destroy_fn_t destroy,
                bsal_actor_receive_fn_t receive,
                int size,
                char *name, char *description, char *author, char *version)
{
    self->identifier = identifier;

    self->init = init;
    self->destroy = destroy;
    self->receive = receive;

    self->size = size;
    self->name = name;
    self->description = description;
    self->author = author;
    self->version = version;
}

void bsal_script_destroy(struct bsal_script *self)
{
    self->identifier = -1;

    self->init = NULL;
    self->destroy = NULL;
    self->receive = NULL;

    self->size = -1;

    self->name = NULL;
    self->description = NULL;
    self->author = NULL;
    self->version = NULL;
}

bsal_actor_init_fn_t bsal_script_get_init(struct bsal_script *self)
{
    return self->init;
}

bsal_actor_destroy_fn_t bsal_script_get_destroy(struct bsal_script *self)
{
    return self->destroy;
}

bsal_actor_receive_fn_t bsal_script_get_receive(struct bsal_script *self)
{
    return self->receive;
}

int bsal_script_identifier(struct bsal_script *self)
{
    return self->identifier;
}

int bsal_script_size(struct bsal_script *self)
{
    return self->size;
}

char *bsal_script_description(struct bsal_script *self)
{
    return self->description;
}

char *bsal_script_name(struct bsal_script *self)
{
    return self->name;
}

char *bsal_script_author(struct bsal_script *self)
{
    return self->author;
}

char *bsal_script_version(struct bsal_script *self)
{
    return self->version;
}
