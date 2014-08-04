
#include "route.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void bsal_route_init(struct bsal_route *self, int tag, int *source, bsal_actor_receive_fn_t callback)
{
    self->tag = tag;
    self->source = source;
    self->callback = callback;
}

void bsal_route_destroy(struct bsal_route *self)
{
    self->tag = -1;
    self->source = NULL;
    self->callback = NULL;
}

bsal_actor_receive_fn_t bsal_route_test(struct bsal_route *self, int tag, int source)
{
    int route_source;

    if (self->tag != tag) {
        return NULL;
    }

    if (self->source != NULL) {
        route_source = *(self->source);

        if (route_source != source) {

#ifdef BSAL_ROUTE_DEBUG
            printf("DEBUG bsal_route_test: route_source %d, source %d\n",
                            route_source, source);
#endif

            return NULL;
        }
    }

    return self->callback;
}

int bsal_route_equals(struct bsal_route *self, struct bsal_route *route)
{
    if (self->tag != route->tag) {
        return 0;
    }

    if (self->source != route->source) {
        return 0;
    }

    return 1;
}

void bsal_route_print(struct bsal_route *self)
{
    int route_source;
    void *callback;

    memcpy(&callback, &self->callback, sizeof(callback));

    route_source = -1;

    if (self->source != NULL) {
        route_source = *(self->source);
    }
    printf("ROUTE Tag %d Source %p (current %d) Handler %p\n",
                    self->tag,
                    (void *)self->source,
                    route_source,
                    callback);
}

int *bsal_route_source(struct bsal_route *self)
{
    return self->source;
}
