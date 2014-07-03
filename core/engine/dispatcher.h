
#ifndef BSAL_DISPATCHER_H
#define BSAL_DISPATCHER_H

#include <core/engine/script.h>
#include <core/structures/map.h>

struct bsal_dispatcher {
    struct bsal_map table;
};

void bsal_dispatcher_init(struct bsal_dispatcher *self);
void bsal_dispatcher_destroy(struct bsal_dispatcher *self);
void bsal_dispatcher_register(struct bsal_dispatcher *self, int tag, bsal_actor_receive_fn_t handler);
int bsal_dispatcher_dispatch(struct bsal_dispatcher *self, struct bsal_actor *actor,
                struct bsal_message *message);
bsal_actor_receive_fn_t bsal_dispatcher_get(struct bsal_dispatcher *self, int tag);

void bsal_dispatcher_print(struct bsal_dispatcher *self);

#endif
