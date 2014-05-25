
#ifndef _BSAL_ACTOR_VTABLE_H
#define _BSAL_ACTOR_VTABLE_H

struct bsal_actor;
struct bsal_message;

typedef void (*bsal_actor_receive_fn_t)(
    struct bsal_actor *actor,
    struct bsal_message *message
);

typedef void (*bsal_actor_init_fn_t)(
    struct bsal_actor *actor
);

typedef void (*bsal_actor_destroy_fn_t)(
    struct bsal_actor *actor
);

struct bsal_actor_vtable {
    bsal_actor_init_fn_t init;
    bsal_actor_destroy_fn_t destroy;
    bsal_actor_receive_fn_t receive;
};

void bsal_actor_vtable_init(struct bsal_actor_vtable *vtable, bsal_actor_init_fn_t init,
                bsal_actor_destroy_fn_t destroy, bsal_actor_receive_fn_t receive);
void bsal_actor_vtable_destroy(struct bsal_actor_vtable *vtable);
bsal_actor_init_fn_t bsal_actor_vtable_get_init(struct bsal_actor_vtable *vtable);
bsal_actor_destroy_fn_t bsal_actor_vtable_get_destroy(struct bsal_actor_vtable *vtable);
bsal_actor_receive_fn_t bsal_actor_vtable_get_receive(struct bsal_actor_vtable *vtable);

#endif
