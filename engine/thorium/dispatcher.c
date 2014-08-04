
#include "dispatcher.h"

#include "message.h"
#include "route.h"
#include "actor.h"

#include <core/structures/vector.h>

#include <stdlib.h>
#include <stdio.h>

/*
#define BSAL_DISPATCHER_DEBUG_10335
*/

void bsal_dispatcher_init(struct bsal_dispatcher *self)
{
    bsal_vector_init(&self->routes, sizeof(struct bsal_route));
}

void bsal_dispatcher_destroy(struct bsal_dispatcher *self)
{
    int i;
    int size;
    struct bsal_route *route;

    size = bsal_vector_size(&self->routes);

    for (i = 0; i < size; i++) {
        route = bsal_vector_at(&self->routes, i);

        bsal_route_destroy(route);
    }
    bsal_vector_destroy(&self->routes);
}

void bsal_dispatcher_register_with_source(struct bsal_dispatcher *self, int tag,
               int source, bsal_actor_receive_fn_t handler)
{
    int i;
    int size;
    struct bsal_route *route;
    struct bsal_route new_route;
    int found;

    /*
     * Check if it is already registered
     */

    found = 0;
    bsal_route_init(&new_route, tag, source, handler);

    size = bsal_vector_size(&self->routes);

    for (i = 0; i < size; i++) {
        route = bsal_vector_at(&self->routes, i);

        if (bsal_route_equals(route, &new_route)) {
            found = 1;
        }
    }

    if (!found) {
        bsal_vector_push_back(&self->routes, &new_route);
    }

    bsal_route_destroy(&new_route);
}

int bsal_dispatcher_dispatch(struct bsal_dispatcher *self, struct bsal_actor *actor,
                struct bsal_message *message)
{
    bsal_actor_receive_fn_t handler;
    int tag;
    int source;

    tag = bsal_message_tag(message);
    source = bsal_message_source(message);

#ifdef BSAL_DISPATCHER_DEBUG_GET
    printf("DEBUG bsal_dispatcher_dispatch Tag %d Source %d\n",
                    tag, source);

    bsal_dispatcher_print(self);
#endif

#ifdef BSAL_DISPATCHER_DEBUG_10335
    if (tag == 10335) {
        printf("DEBUG bsal_dispatcher_dispatch 10335, available %d\n",
                        bsal_vector_size(&self->tags));
        bsal_dispatcher_print(self);
    }
#endif

    handler = bsal_dispatcher_get(self, tag, source);

    if (handler == NULL) {

#ifdef BSAL_DISPATCHER_DEBUG_10335
        if (tag == 10335) {
            printf("DEBUG 10335 is not registered.\n");
            bsal_dispatcher_print(self);
        }
#endif

        return 0;
    }

    handler(actor, message);

    return 1;
}

bsal_actor_receive_fn_t bsal_dispatcher_get(struct bsal_dispatcher *self, int tag, int source)
{
    int i;
    int size;
    bsal_actor_receive_fn_t callback;
    bsal_actor_receive_fn_t callback_without_source;
    int callback_without_source_index;
    struct bsal_route *route;

    size = bsal_vector_size(&self->routes);

    callback = NULL;
    callback_without_source = NULL;
    callback_without_source_index = -1;

    for (i = 0; i < size; i++) {
        route = bsal_vector_at(&self->routes, i);

        callback = bsal_route_test(route, tag, source);

        if (callback != NULL) {

            if (bsal_route_source(route) == BSAL_ACTOR_ANYBODY) {
                callback_without_source = callback;
                callback_without_source_index = i;
            } else {
#ifdef BSAL_DISPATCHER_DEBUG_GET
                printf("Got callback %d\n", i);
#endif
                return callback;
            }
        }
    }

    if (callback_without_source != NULL) {

#ifdef BSAL_DISPATCHER_DEBUG_GET
        printf("Got callback %d\n", callback_without_source_index);
#endif
        return callback_without_source;
    }

#ifdef BSAL_DISPATCHER_DEBUG_GET
    printf("No callback matched.\n");
#endif

    return NULL;
}

void bsal_dispatcher_print(struct bsal_dispatcher *self)
{
    int i;
    int size;
    struct bsal_route *route;

    printf("DEBUG Dispatcher handlers (%d):",
                    (int)bsal_vector_size(&self->routes));

    size = bsal_vector_size(&self->routes);

    for (i = 0; i < size; i++) {
        route = bsal_vector_at(&self->routes, i);

        printf("# %d ", i);
        bsal_route_print(route);

    }
}
