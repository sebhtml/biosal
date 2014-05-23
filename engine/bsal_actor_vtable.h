

#ifndef _BIOSAL_ACTOR_VTABLE_H
#define _BIOSAL_ACTOR_VTABLE_H

struct bsal_actor;

typedef void (*bsal_receive_fn_t)(
    struct bsal_actor *actor,
    struct bsal_message *message
);

struct bsal_actor_vtable {
    bsal_receive_fn_t receive;
};

void bsal_actor_vtable_construct(struct bsal_actor_vtable *vtable, bsal_receive_fn_t receive);
void bsal_actor_vtable_destruct(struct bsal_actor_vtable *vtable);
bsal_receive_fn_t bsal_actor_vtable_receive(struct bsal_actor_vtable *vtable);

#endif 
