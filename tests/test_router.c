
#include "test.h"

#include <engine/thorium/topology/router.h>

#include <string.h>
#include <inttypes.h>

int main(int argc, char **argv)
{
    BEGIN_TESTS();

    struct thorium_router router;
    int destination;
    int current;
    int next;
    int source;
    int hops;

    thorium_router_init(&router, 256, TOPOLOGY_POLYTOPE);

    current = 3;
    source = -1;
    destination = 255;

    hops = 0;

    while (current != destination && hops < 100) {
        next = thorium_router_get_next_rank_in_route(&router, source, current, destination);
        current = next;
    }

    TEST_INT_EQUALS(current, destination);

    /*
    printf("%d -> ... %d -> %d -> ... -> %d\n",
                    source, current, next, destination);
                    */

    thorium_router_destroy(&router);

    END_TESTS();

    return 0;
}
