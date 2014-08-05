
#ifndef BSAL_DISPATCHER_H
#define BSAL_DISPATCHER_H

#include "script.h"

#include <core/structures/map.h>

/*
 * A message dispatcher.
 *
 * A route is selected like this:
 *
 * Check tag
 * Check source
 * return handler
 *
 */
struct bsal_dispatcher {
    struct bsal_map routes;
};

void bsal_dispatcher_init(struct bsal_dispatcher *self);
void bsal_dispatcher_destroy(struct bsal_dispatcher *self);

void bsal_dispatcher_register_route(struct bsal_dispatcher *self, int tag, bsal_actor_receive_fn_t handler,
                int source, int *actual, int expected);

int bsal_dispatcher_dispatch(struct bsal_dispatcher *self, struct bsal_actor *actor,
                struct bsal_message *message);
bsal_actor_receive_fn_t bsal_dispatcher_get(struct bsal_dispatcher *self, int tag, int source);

void bsal_dispatcher_print(struct bsal_dispatcher *self);

#endif
