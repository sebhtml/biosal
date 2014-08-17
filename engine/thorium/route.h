
#ifndef _BSAL_ROUTE_H
#define _BSAL_ROUTE_H

#include "script.h"

#define BSAL_ROUTE_CONDITION_NONE 0
#define BSAL_ROUTE_CONDITION_TRUE 1
#define BSAL_ROUTE_CONDITION_FALSE 2

/*
 * A message route
 *
 * Routes are indexed by tags, then by sources.
 */
struct thorium_route {
    int *actual_value;
    int expected_value;
    thorium_actor_receive_fn_t handler;
};

void thorium_route_init(struct thorium_route *self, int *actual_value, int expected_value,
                thorium_actor_receive_fn_t handler);
void thorium_route_destroy(struct thorium_route *self);
int thorium_route_test(struct thorium_route *self);
thorium_actor_receive_fn_t thorium_route_handler(struct thorium_route *self);

int thorium_route_equals(struct thorium_route *self, struct thorium_route *route);


#endif
