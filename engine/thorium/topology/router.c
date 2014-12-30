
#include "router.h"

#include <stdio.h>

/*
#define CONFIG_DISABLE_ROUTER
*/

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
    int node;

#ifdef CONFIG_DISABLE_ROUTER
    return destination;
#endif

    node = thorium_polytope_get_next_rank_in_route(&self->polytope,
                    source, current, destination);


    if (node == -1)
        return destination;

    return node;
}

void thorium_router_print(struct thorium_router *self)
{
    thorium_polytope_print(&self->polytope);
}
