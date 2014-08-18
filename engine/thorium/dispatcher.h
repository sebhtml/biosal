
#ifndef THORIUM_DISPATCHER_H
#define THORIUM_DISPATCHER_H

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
struct thorium_dispatcher {
    struct bsal_map routes;
};

void thorium_dispatcher_init(struct thorium_dispatcher *self);
void thorium_dispatcher_destroy(struct thorium_dispatcher *self);

void thorium_dispatcher_add_route(struct thorium_dispatcher *self, int tag, thorium_actor_receive_fn_t handler,
                int source, int *actual, int expected);

int thorium_dispatcher_dispatch(struct thorium_dispatcher *self, struct thorium_actor *actor,
                struct thorium_message *message);
thorium_actor_receive_fn_t thorium_dispatcher_get(struct thorium_dispatcher *self, int tag, int source);

void thorium_dispatcher_print(struct thorium_dispatcher *self);

#endif
