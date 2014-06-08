
#include "dispatcher.h"
#include "message.h"

#include <stdlib.h>
#include <stdio.h>

/*
#define BSAL_DISPATCHER_DEBUG_10335
*/

void bsal_dispatcher_init(struct bsal_dispatcher *self)
{
    bsal_vector_init(&self->tags, sizeof(int));
    bsal_vector_init(&self->handlers, sizeof(bsal_actor_receive_fn_t));
}

void bsal_dispatcher_destroy(struct bsal_dispatcher *self)
{
    bsal_vector_destroy(&self->tags);
    bsal_vector_destroy(&self->handlers);
}

void bsal_dispatcher_register(struct bsal_dispatcher *self, int tag, bsal_actor_receive_fn_t handler)
{
    if (bsal_dispatcher_get(self, tag) != NULL) {
        return;
    }

    bsal_vector_push_back(&self->tags, &tag);
    bsal_vector_push_back(&self->handlers, &handler);

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
    int index;
    bsal_actor_receive_fn_t *bucket;

    index = bsal_vector_index_of(&self->tags, &tag);

    if (index < 0) {
        return NULL;
    }

    bucket = (bsal_actor_receive_fn_t *)bsal_vector_at(&self->handlers, index);

    return *bucket;
}

void bsal_dispatcher_print(struct bsal_dispatcher *self)
{
    int i;
    int tag;
    void *handler;

    printf("DEBUG DATA Dispatcher handlers (%d):",
                    bsal_vector_size(&self->tags));

    for (i = 0; i < bsal_vector_size(&self->tags); i++) {
        tag = bsal_vector_at_as_int(&self->tags, i);
        handler = bsal_vector_at_as_void_pointer(&self->handlers, i);

        printf(" (%d %p)", tag, handler);
    }
    printf("\n");
}
