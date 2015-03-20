
#include "script.h"

#include <stdlib.h>

#include <stdio.h>

void thorium_script_init(struct thorium_script *self, int identifier,
                thorium_actor_init_fn_t init,
                thorium_actor_destroy_fn_t destroy,
                thorium_actor_receive_fn_t receive,
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

void thorium_script_destroy(struct thorium_script *self)
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

thorium_actor_init_fn_t thorium_script_get_init(struct thorium_script *self)
{
    return self->init;
}

thorium_actor_destroy_fn_t thorium_script_get_destroy(struct thorium_script *self)
{
    return self->destroy;
}

thorium_actor_receive_fn_t thorium_script_get_receive(struct thorium_script *self)
{
    return self->receive;
}

int thorium_script_identifier(struct thorium_script *self)
{
    return self->identifier;
}

int thorium_script_size(struct thorium_script *self)
{
    return self->size;
}

char *thorium_script_description(struct thorium_script *self)
{
    return self->description;
}

char *thorium_script_name(struct thorium_script *self)
{
    return self->name;
}

char *thorium_script_author(struct thorium_script *self)
{
    return self->author;
}

char *thorium_script_version(struct thorium_script *self)
{
    return self->version;
}

void thorium_script_check_sanity(struct thorium_script *self)
{
    if (self->name == NULL) {
        printf("Error: script has invalid name...\n");
    }
}
