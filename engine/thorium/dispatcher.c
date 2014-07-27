
#include "message.h"
#include "dispatcher.h"

#include <core/structures/map_iterator.h>

#include <stdlib.h>
#include <stdio.h>

/*
#define BSAL_DISPATCHER_DEBUG_10335
*/

void bsal_dispatcher_init(struct bsal_dispatcher *self)
{
    bsal_map_init(&self->table, sizeof(int), sizeof(bsal_actor_receive_fn_t));
}

void bsal_dispatcher_destroy(struct bsal_dispatcher *self)
{
    bsal_map_destroy(&self->table);
}

void bsal_dispatcher_register(struct bsal_dispatcher *self, int tag, bsal_actor_receive_fn_t handler)
{
    bsal_actor_receive_fn_t *bucket;

    if (bsal_dispatcher_get(self, tag) != NULL) {
        return;
    }

    bucket = (bsal_actor_receive_fn_t *)bsal_map_add(&self->table, &tag);

    *bucket = handler;

#ifdef BSAL_DISPATCHER_DEBUG_10335
    if (tag == 10335) {
        printf("DEBUG bsal_dispatcher_register OK tag %d, available: %d\n", tag,
                        bsal_vector_size(&self->tags));
        bsal_dispatcher_print(self);
    }
#endif
}

int bsal_dispatcher_dispatch(struct bsal_dispatcher *self, struct bsal_actor *actor,
                struct bsal_message *message)
{
    bsal_actor_receive_fn_t handler;
    int tag;

    tag = bsal_message_tag(message);

#ifdef BSAL_DISPATCHER_DEBUG_10335
    if (tag == 10335) {
        printf("DEBUG bsal_dispatcher_dispatch 10335, available %d\n",
                        bsal_vector_size(&self->tags));
        bsal_dispatcher_print(self);
    }
#endif

    handler = bsal_dispatcher_get(self, tag);

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

bsal_actor_receive_fn_t bsal_dispatcher_get(struct bsal_dispatcher *self, int tag)
{
    bsal_actor_receive_fn_t *bucket;

    bucket = (bsal_actor_receive_fn_t *)bsal_map_get(&self->table, &tag);

    if (bucket == NULL) {
        return NULL;
    }

    return *bucket;
}

void bsal_dispatcher_print(struct bsal_dispatcher *self)
{
    int *tag;
    void *handler;
    struct bsal_map_iterator iterator;

    printf("DEBUG Dispatcher handlers (%d):",
                    (int)bsal_map_size(&self->table));

    bsal_map_iterator_init(&iterator, &self->table);

    while (bsal_map_iterator_has_next(&iterator)) {

        bsal_map_iterator_next(&iterator, (void **)&tag, (void**)&handler);

        printf(" (%d %p)", *tag, handler);
    }
    printf("\n");

    bsal_map_iterator_destroy(&iterator);
}
