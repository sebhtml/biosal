
#ifndef THORIUM_ROUTER_H
#define THORIUM_ROUTER_H

#include "polytope.h"

/**
 * A router that uses a virtual or physical topology.
 */
struct thorium_router {
    int topology;
    int size;

    struct thorium_polytope polytope;
};

void thorium_router_init(struct thorium_router *self, int size, int topology);
void thorium_router_destroy(struct thorium_router *self);

int thorium_router_get_next_rank_in_route(struct thorium_router *self,
                int source, int current, int destination);

#endif
