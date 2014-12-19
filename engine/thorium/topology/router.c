
#include "router.h"

#include <stdio.h>

void thorium_router_init(struct thorium_router *self, int size, int topology)
{
        /*
    printf("INIT\n");
    */
    thorium_polytope_init(&self->polytope, size);
}

void thorium_router_destroy(struct thorium_router *self)
{
    thorium_polytope_destroy(&self->polytope);
}

int thorium_router_get_next_rank_in_route(struct thorium_router *self,
                int source, int current, int destination)
{
    return thorium_polytope_get_next_rank_in_route(&self->polytope,
                    source, current, destination);
}


