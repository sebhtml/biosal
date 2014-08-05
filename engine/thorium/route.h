
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
struct bsal_route {
    int *actual_value;
    int expected_value;
    bsal_actor_receive_fn_t handler;
};

void bsal_route_init(struct bsal_route *self, int *actual_value, int expected_value,
                bsal_actor_receive_fn_t handler);
void bsal_route_destroy(struct bsal_route *self);
int bsal_route_test(struct bsal_route *self);
bsal_actor_receive_fn_t bsal_route_handler(struct bsal_route *self);

int bsal_route_equals(struct bsal_route *self, struct bsal_route *route);


#endif
