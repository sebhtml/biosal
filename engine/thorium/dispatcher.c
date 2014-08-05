
#include "dispatcher.h"

#include "message.h"
#include "actor.h"

#include <core/structures/map.h>
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

void bsal_dispatcher_register_with_source(struct bsal_dispatcher *self, int tag,
               int source, bsal_actor_receive_fn_t handler)
{
    struct bsal_map *map;
    bsal_actor_receive_fn_t *bucket;

    map = bsal_map_get(&self->routes, &tag);

    /*
     * Initialize the map if it does not exist
     */
    if (map == NULL) {
        map = bsal_map_add(&self->routes, &tag);

        bsal_map_init(map, sizeof(int), sizeof(bsal_actor_receive_fn_t));
    }

    bucket = bsal_map_get(map, &source);

    /*
     * Create the bucket if it is not there.
     */
    if (bucket == NULL) {
        bucket = bsal_map_add(map, &source);
    }

    *bucket = handler;
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
    bsal_actor_receive_fn_t *bucket;
    bsal_actor_receive_fn_t handler;
    int any_source;

    map = bsal_map_get(&self->routes, &tag);

    /*
     * This tag is not configured
     */
    if (map == NULL) {
        return NULL;
    }

    bucket = bsal_map_get(map, &source);

    /*
     * This source is not
     * configured
     */
    if (bucket == NULL) {

        /* 
         * Check if a wildcard was registered
         */
        any_source = BSAL_ACTOR_ANYBODY;
        bucket = bsal_map_get(map, &any_source);
    }

    /* BSAL_ACTOR_ANYBODY is not registered.
     */
    if (bucket == NULL) {
        return NULL;
    }
    handler = *bucket;

    return handler;
}

void bsal_dispatcher_print(struct bsal_dispatcher *self)
{
}
