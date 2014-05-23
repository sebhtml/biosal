

#ifndef _BIOSAL_ACTOR_H
#define _BIOSAL_ACTOR_H

#include "bsal_message.h"
#include "bsal_actor_vtable.h"

/**
 * TODO: the actor attribute should be a void *
 */
struct bsal_actor {
    /* bsal_actor_vtable *vtable; */
    bsal_receive_fn_t receive;
    void *actor;
    int name;
};
typedef struct bsal_actor bsal_actor_t;

void bsal_actor_construct(struct bsal_actor *bsal_actor, void *actor, bsal_receive_fn_t receive);
void bsal_actor_destruct(struct bsal_actor *bsal_actor);
int bsal_actor_name(struct bsal_actor *bsal_actor);
void *bsal_actor_actor(struct bsal_actor *bsal_actor);
bsal_receive_fn_t bsal_actor_handler(struct bsal_actor *bsal_actor);
void bsal_actor_set_name(struct bsal_actor *actor, int name);
void bsal_actor_print(struct bsal_actor *actor);

#endif


