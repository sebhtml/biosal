
#include "dispatcher.h"

#include "message.h"
#include "actor.h"
#include "route.h"

#include <core/structures/map.h>
#include <core/structures/vector.h>
#include <core/structures/map_iterator.h>

#include <stdlib.h>
#include <stdio.h>

/*
#define THORIUM_DISPATCHER_DEBUG_10335
*/

void thorium_dispatcher_init(struct thorium_dispatcher *self,
                struct core_memory_pool *pool)
{
    self->pool = pool;

    core_map_init(&self->routes, sizeof(int), sizeof(struct core_map));
    core_map_set_memory_pool(&self->routes, self->pool);
}

void thorium_dispatcher_destroy(struct thorium_dispatcher *self)
{
    struct core_map_iterator iterator;
    struct core_map_iterator iterator2;
    struct core_map *map;
    struct core_vector *vector;

    core_map_iterator_init(&iterator, &self->routes);

    /*
     * For each tag, iterate over each source and destroy each vector
     * of routes.
     */
    while (core_map_iterator_has_next(&iterator)) {
        core_map_iterator_next(&iterator, NULL, (void **)&map);

        core_map_iterator_init(&iterator2, map);

        while (core_map_iterator_has_next(&iterator2)) {
            core_map_iterator_next(&iterator2, NULL, (void **)&vector);

            /*
             * Destroy routes
             */
            core_vector_destroy(vector);
        }

        core_map_iterator_destroy(&iterator2);

        core_map_destroy(map);
    }

    core_map_iterator_destroy(&iterator);

    core_map_destroy(&self->routes);
}

void thorium_dispatcher_add_action(struct thorium_dispatcher *self, int tag,
               thorium_actor_receive_fn_t handler,
               int source,
               int *actual,
               int expected)
{
    struct core_map *map;
    struct core_vector *vector;
    struct thorium_route *route;
    struct thorium_route new_route;
    int size;
    int i;
    int found;

    map = core_map_get(&self->routes, &tag);

    /*
     * Initialize the map if it does not exist
     */
    if (map == NULL) {
        map = core_map_add(&self->routes, &tag);

        core_map_init(map, sizeof(int), sizeof(struct core_vector));
        core_map_set_memory_pool(map, self->pool);
    }

    vector = core_map_get(map, &source);

    /*
     * Create the bucket if it is not there.
     */
    if (vector == NULL) {
        vector = core_map_add(map, &source);

        core_vector_init(vector, sizeof(struct thorium_route));
        core_vector_set_memory_pool(vector, self->pool);
    }

    /* Add the route in the vector
     */

    thorium_route_init(&new_route, actual, expected, handler);

    /* Check if it is there already
     */

    size = core_vector_size(vector);
    found = 0;

    for (i = 0; i < size; i++) {
        route = core_vector_at(vector, i);

        if (thorium_route_equals(route, &new_route)) {
            found = 1;
            break;
        }
    }

    if (!found) {
        core_vector_push_back(vector, &new_route);
    }

    thorium_route_destroy(&new_route);
}

int thorium_dispatcher_dispatch(struct thorium_dispatcher *self, struct thorium_actor *actor,
                struct thorium_message *message)
{
    thorium_actor_receive_fn_t handler;
    int tag;
    int source;

    tag = thorium_message_action(message);
    source = thorium_message_source(message);

#ifdef THORIUM_DISPATCHER_DEBUG_GET
    printf("DEBUG thorium_dispatcher_dispatch Tag %d Source %d\n",
                    tag, source);

    thorium_dispatcher_print(self);
#endif

#ifdef THORIUM_DISPATCHER_DEBUG_10335
    if (tag == 10335) {
        printf("DEBUG thorium_dispatcher_dispatch 10335, available %d\n",
                        core_vector_size(&self->tags));
        thorium_dispatcher_print(self);
    }
#endif

    /*
     * First try to get a route using
     * the tag and the source.
     *
     * If that does not work, try with the tag and
     * with the source wildcard THORIUM_ACTOR_ANYBODY
     */
    handler = thorium_dispatcher_get(self, tag, source);

    /*
     * If there is no route for this source,
     * try with the wild card THORIUM_ACTOR_ANYBODY to see
     * if this is registered.
     *
     */
    if (handler == NULL) {

        handler = thorium_dispatcher_get(self, tag, THORIUM_ACTOR_ANYBODY);
    }

    if (handler == NULL) {

        /*
         * There is no default route for this tag and this source.
         * In fact, there is no route for this tag and
         * THORIUM_ACTOR_ANYBODY.
         *
         */
#ifdef THORIUM_DISPATCHER_DEBUG_10335
        if (tag == 10335) {
            printf("DEBUG 10335 is not registered.\n");
            thorium_dispatcher_print(self);
        }
#endif

        return 0;
    }

    handler(actor, message);

    return 1;
}

thorium_actor_receive_fn_t thorium_dispatcher_get(struct thorium_dispatcher *self, int tag, int source)
{
    struct core_map *map;
    struct core_vector *vector;
    struct thorium_route *route;
    int i;
    int size;
    thorium_actor_receive_fn_t handler_with_condition;
    thorium_actor_receive_fn_t handler_without_condition;

    handler_with_condition = NULL;
    handler_without_condition = NULL;

    map = core_map_get(&self->routes, &tag);

    /*
     * This tag is not configured
     */
    if (map == NULL) {
        return NULL;
    }

    vector = core_map_get(map, &source);

    /*
     * This source is not
     * configured
     */
    if (vector == NULL) {

        return NULL;
    }

    /*
     * Pick up the first route with a satisfied condition
     */

    size = core_vector_size(vector);

    for (i = 0; i < size; i++) {

        route = core_vector_at(vector, i);

        if (thorium_route_test(route) == THORIUM_ROUTE_CONDITION_TRUE) {
            handler_with_condition = thorium_route_handler(route);
        } else if (thorium_route_test(route) == THORIUM_ROUTE_CONDITION_NONE) {
            handler_without_condition = thorium_route_handler(route);
        }

        /* Otherwise it is THORIUM_ROUTE_CONDITION_FALSE
         */
    }

    /*
     * THORIUM_ROUTE_CONDITION_TRUE has priority
     */

    if (handler_with_condition != NULL) {
        return handler_with_condition;
    }

    if (handler_without_condition != NULL) {
        return handler_without_condition;
    }

    return NULL;
}

void thorium_dispatcher_print(struct thorium_dispatcher *self)
{
}
