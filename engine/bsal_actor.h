

#ifndef _BIOSAL_ACTOR_H
#define _BIOSAL_ACTOR_H

#include "bsal_message.h"

struct bsal_actor;

typedef void (*bsal_receive_fn_t)(
    struct bsal_actor *bsal_actor,
    struct bsal_message *message
);

struct bsal_actor {
    bsal_receive_fn_t receive;
    struct bsal_actor *actor;
};
typedef struct bsal_actor bsal_actor_t;

void bsal_actor_construct(struct bsal_actor *bsal_actor, void *actor, bsal_receive_fn_t receive);
void bsal_actor_destruct(struct bsal_actor *bsal_actor);

#endif


