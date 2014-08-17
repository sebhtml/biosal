
#include "route.h"

#include <stdlib.h>

void thorium_route_init(struct thorium_route *self, int *actual_value, int expected_value, thorium_actor_receive_fn_t handler)
{
    self->actual_value = actual_value;
    self->expected_value = expected_value;
    self->handler = handler;
}

void thorium_route_destroy(struct thorium_route *self)
{
    self->actual_value = NULL;
    self->expected_value = -1;
    self->handler = NULL;
}

int thorium_route_test(struct thorium_route *self)
{
    if (self->actual_value == NULL) {
        return BSAL_ROUTE_CONDITION_NONE;
    }

    if (*(self->actual_value) == self->expected_value) {
        return BSAL_ROUTE_CONDITION_TRUE;
    }

    return BSAL_ROUTE_CONDITION_FALSE;
}

thorium_actor_receive_fn_t thorium_route_handler(struct thorium_route *self)
{
    return self->handler;
}

int thorium_route_equals(struct thorium_route *self, struct thorium_route *route)
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
