
#ifndef BSAL_ACTOR_VTABLE_H
#define BSAL_ACTOR_VTABLE_H

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

struct bsal_script {
    bsal_actor_init_fn_t init;
    bsal_actor_destroy_fn_t destroy;
    bsal_actor_receive_fn_t receive;
    int name;
    int size;
    char *description;
};

void bsal_script_init(struct bsal_script *script, bsal_actor_init_fn_t init,
                bsal_actor_destroy_fn_t destroy, bsal_actor_receive_fn_t receive);
void bsal_script_destroy(struct bsal_script *script);
bsal_actor_init_fn_t bsal_script_get_init(struct bsal_script *script);
bsal_actor_destroy_fn_t bsal_script_get_destroy(struct bsal_script *script);
bsal_actor_receive_fn_t bsal_script_get_receive(struct bsal_script *script);
int bsal_script_name(struct bsal_script *script);
char *bsal_script_description(struct bsal_script *script);
int bsal_script_size(struct bsal_script *script);

#endif
