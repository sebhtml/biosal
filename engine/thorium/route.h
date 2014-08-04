
#ifndef BSAL_ROUTE
#define BSAL_ROUTE

#include "script.h"

struct bsal_route {
    int tag;
    int source;
    bsal_actor_receive_fn_t callback;
};

void bsal_route_init(struct bsal_route *self, int tag, int source, bsal_actor_receive_fn_t callback);
void bsal_route_destroy(struct bsal_route *self);

bsal_actor_receive_fn_t bsal_route_test(struct bsal_route *self, int tag, int source);

int bsal_route_equals(struct bsal_route *self, struct bsal_route *route);

void bsal_route_print(struct bsal_route *self);
int bsal_route_source(struct bsal_route *self);

#endif
