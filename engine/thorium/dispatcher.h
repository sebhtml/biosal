
#ifndef THORIUM_DISPATCHER_H
#define THORIUM_DISPATCHER_H

#include "script.h"

#include <core/structures/map.h>

struct core_memory_pool;

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
    struct core_map routes;
    struct core_memory_pool *pool;
};

void thorium_dispatcher_init(struct thorium_dispatcher *self, struct core_memory_pool *pool);
void thorium_dispatcher_destroy(struct thorium_dispatcher *self);

void thorium_dispatcher_add_action(struct thorium_dispatcher *self, int tag, thorium_actor_receive_fn_t handler,
                int source, int *actual, int expected);

int thorium_dispatcher_dispatch(struct thorium_dispatcher *self, struct thorium_actor *actor,
                struct thorium_message *message);

void thorium_dispatcher_print(struct thorium_dispatcher *self);

#endif
