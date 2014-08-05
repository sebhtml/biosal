
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
#define BSAL_DISPATCHER_DEBUG_10335
*/

void bsal_dispatcher_init(struct bsal_dispatcher *self)
{
    bsal_map_init(&self->routes, sizeof(int), sizeof(struct bsal_map));
}

void bsal_dispatcher_destroy(struct bsal_dispatcher *self)
{
    struct bsal_map_iterator iterator;
    struct bsal_map *map;

    bsal_map_iterator_init(&iterator, &self->routes);

    while (bsal_map_iterator_has_next(&iterator)) {
        bsal_map_iterator_next(&iterator, NULL, (void **)&map);

        bsal_map_destroy(map);
    }

    bsal_map_iterator_destroy(&iterator);

    bsal_map_destroy(&self->routes);
}

void bsal_dispatcher_add_route(struct bsal_dispatcher *self, int tag,
               bsal_actor_receive_fn_t handler,
               int source,
               int *actual,
               int expected)
{
    struct bsal_map *map;
    struct bsal_vector *vector;
    struct bsal_route *route;
    struct bsal_route new_route;
    int size;
    int i;
    int found;

    map = bsal_map_get(&self->routes, &tag);

    /*
     * Initialize the map if it does not exist
     */
    if (map == NULL) {
        map = bsal_map_add(&self->routes, &tag);

        bsal_map_init(map, sizeof(int), sizeof(struct bsal_vector));
    }

    vector = bsal_map_get(map, &source);

    /*
     * Create the bucket if it is not there.
     */
    if (vector == NULL) {
        vector = bsal_map_add(map, &source);

        bsal_vector_init(vector, sizeof(struct bsal_route));
    }

    /* Add the route in the vector
     */

    bsal_route_init(&new_route, actual, expected, handler);

    /* Check if it is there already
     */

    size = bsal_vector_size(vector);
    found = 0;

    for (i = 0; i < size; i++) {
        route = bsal_vector_at(vector, i);

        if (bsal_route_equals(route, &new_route)) {
            found = 1;
            break;
        }
    }

    if (!found) {
        bsal_vector_push_back(vector, &new_route);
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
    struct bsal_map *map;
    struct bsal_vector *vector;
    struct bsal_route *route;
    int any_source;
    int i;
    int size;
    bsal_actor_receive_fn_t handler_with_condition;
    bsal_actor_receive_fn_t handler_without_condition;

    handler_with_condition = NULL;
    handler_without_condition = NULL;

    map = bsal_map_get(&self->routes, &tag);

    /*
     * This tag is not configured
     */
    if (map == NULL) {
        return NULL;
    }

    vector = bsal_map_get(map, &source);

    /*
     * This source is not
     * configured
     */
    if (vector == NULL) {

        /*
         * Check if a wildcard was registered
         */
        any_source = BSAL_ACTOR_ANYBODY;
        vector = bsal_map_get(map, &any_source);
    }

    /* BSAL_ACTOR_ANYBODY is not registered.
     */
    if (vector == NULL) {
        return NULL;
    }

    /*
     * Pick up the first route with a satisfied condition
     */

    size = bsal_vector_size(vector);

    for (i = 0; i < size; i++) {

        route = bsal_vector_at(vector, i);

        if (bsal_route_test(route) == BSAL_ROUTE_CONDITION_TRUE) {
            handler_with_condition = bsal_route_handler(route);
        } else if (bsal_route_test(route) == BSAL_ROUTE_CONDITION_NONE) {
            handler_without_condition = bsal_route_handler(route);
        }

        /* Otherwise it is BSAL_ROUTE_CONDITION_FALSE
         */
    }

    /*
     * BSAL_ROUTE_CONDITION_TRUE has priority
     */

    if (handler_with_condition != NULL) {
        return handler_with_condition;
    }

    if (handler_without_condition != NULL) {
        return handler_without_condition;
    }


    return NULL;
}

void bsal_dispatcher_print(struct bsal_dispatcher *self)
{
}
