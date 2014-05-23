

#ifndef _BIOSAL_ACTOR_VTABLE_H
#define _BIOSAL_ACTOR_VTABLE_H

struct bsal_actor;
struct bsal_message;

typedef void (*bsal_actor_receive_fn_t)(
    struct bsal_actor *actor,
    struct bsal_message *message
);

typedef void (*bsal_actor_construct_fn_t)(
    struct bsal_actor *actor
);

typedef void (*bsal_actor_destruct_fn_t)(
    struct bsal_actor *actor
);

struct bsal_actor_vtable {
    bsal_actor_construct_fn_t construct;
    bsal_actor_destruct_fn_t destruct;
    bsal_actor_receive_fn_t receive;
};

void bsal_actor_vtable_construct(struct bsal_actor_vtable *vtable, bsal_actor_construct_fn_t construct,
                bsal_actor_destruct_fn_t destruct, bsal_actor_receive_fn_t receive);
void bsal_actor_vtable_destruct(struct bsal_actor_vtable *vtable);
bsal_actor_construct_fn_t bsal_actor_vtable_get_construct(struct bsal_actor_vtable *vtable);
bsal_actor_destruct_fn_t bsal_actor_vtable_get_destruct(struct bsal_actor_vtable *vtable);
bsal_actor_receive_fn_t bsal_actor_vtable_get_receive(struct bsal_actor_vtable *vtable);

#endif 
