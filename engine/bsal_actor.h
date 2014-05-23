
#ifndef _BSAL_ACTOR_H
#define _BSAL_ACTOR_H

#include "bsal_message.h"
#include "bsal_actor_vtable.h"

enum {
    BSAL_START = 1000 /* FIRST_TAG */ /* LAST_TAG */
};

struct bsal_node;

/*
 * the actor attribute is a void *
 */
struct bsal_actor {
    /* struct bsal_actor_vtable *vtable; */
    bsal_actor_receive_fn_t receive;
    void *actor;
    int name;
    int dead;
    struct bsal_node *node;
};
typedef struct bsal_actor bsal_actor_t;

void bsal_actor_construct(struct bsal_actor *actor, void *pointer,
                bsal_actor_receive_fn_t receive);
void bsal_actor_destruct(struct bsal_actor *actor);

int bsal_actor_name(struct bsal_actor *actor);
void *bsal_actor_actor(struct bsal_actor *actor);
void bsal_actor_set_name(struct bsal_actor *actor, int name);
void bsal_actor_set_node(struct bsal_actor *actor, struct bsal_node *node);
void bsal_actor_print(struct bsal_actor *actor);
int bsal_actor_size(struct bsal_actor *actor);
int bsal_actor_dead(struct bsal_actor *actor);
void bsal_actor_die(struct bsal_actor *actor);

bsal_actor_construct_fn_t bsal_actor_get_construct(struct bsal_actor *actor);
bsal_actor_destruct_fn_t bsal_actor_get_destruct(struct bsal_actor *actor);
bsal_actor_receive_fn_t bsal_actor_get_receive(struct bsal_actor *actor);
void bsal_actor_send(struct bsal_actor *actor, int name, struct bsal_message *message);

struct bsal_node *bsal_actor_node(struct bsal_actor *actor);
int bsal_actor_spawn(struct bsal_actor *actor, void *pointer, bsal_actor_receive_fn_t receive);

#endif
