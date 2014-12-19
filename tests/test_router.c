
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
    int size = 256;
    int i;
    int j;

    thorium_router_init(&router, size, TOPOLOGY_POLYTOPE);

    current = 3;
    source = -1;
    destination = 255;

    hops = 0;

    while (current != destination && hops < 100) {
        next = thorium_router_get_next_rank_in_route(&router, source, current, destination);
        current = next;
        ++hops;
    }

    TEST_INT_EQUALS(current, destination);
    TEST_INT_IS_LOWER_THAN_OR_EQUAL(hops, 4);

    /*
    printf("%d -> ... %d -> %d -> ... -> %d\n",
                    source, current, next, destination);
                    */

    /*
     * Make sure that all pairs of nodes have a route with at most
     * 2 hops since 256 is 16 * 16.
     */

    for (i = 0; i < size; ++i) {
        for (j = 0; j < size; ++j) {
            current = i;
            destination = j;
            hops = 0;
            while (current != destination && hops < 10) {
                next = thorium_router_get_next_rank_in_route(&router, -1, current, destination);
                current = next;
                ++hops;
            }

            TEST_INT_IS_LOWER_THAN_OR_EQUAL(hops, 2);
            TEST_INT_EQUALS(current, destination);
        }
    }

    thorium_router_destroy(&router);

    END_TESTS();

    return 0;
}
