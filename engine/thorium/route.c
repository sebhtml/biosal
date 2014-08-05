
#include "route.h"

#include <stdlib.h>

void bsal_route_init(struct bsal_route *self, int *actual_value, int expected_value, bsal_actor_receive_fn_t handler)
{
    self->actual_value = actual_value;
    self->expected_value = expected_value;
    self->handler = handler;
}

void bsal_route_destroy(struct bsal_route *self)
{
    self->actual_value = NULL;
    self->expected_value = -1;
    self->handler = NULL;
}

int bsal_route_test(struct bsal_route *self)
{
    if (self->actual_value == NULL) {
        return BSAL_ROUTE_CONDITION_NONE;
    }

    if (*(self->actual_value) == self->expected_value) {
        return BSAL_ROUTE_CONDITION_TRUE;
    }

    return BSAL_ROUTE_CONDITION_FALSE;
}

bsal_actor_receive_fn_t bsal_route_handler(struct bsal_route *self)
{
    return self->handler;
}

int bsal_route_equals(struct bsal_route *self, struct bsal_route *route)
{
    if (self->expected_value != route->expected_value) {
        return 0;
    }

    if (self->actual_value != route->actual_value) {
        return 0;
    }

    if (self->handler != route->handler) {
        return 0;
    }

    return 1;
}
