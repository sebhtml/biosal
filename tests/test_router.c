
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
    int count_0_hops;
    int count_1_hops;
    int count_2_hops;

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

    count_0_hops = 0;
    count_1_hops = 0;
    count_2_hops = 0;

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

            if (hops == 0)
                ++count_0_hops;
            else if (hops == 1)
                ++count_1_hops;
            else if (hops == 2)
                ++count_2_hops;
        }
    }

    TEST_INT_EQUALS(count_0_hops, size);
    TEST_INT_EQUALS(size * size, count_0_hops + count_1_hops + count_2_hops);

    thorium_router_destroy(&router);

    /*
     * Test with 125 nodes.
     * The code should find (5 * 5 * 5).
     */
    {
        struct thorium_router router;
        int size = 125;
        int source = 0;
        int destination = 124;
        int hops = 0;
        int current = source;
        int next = -1;

        thorium_router_init(&router, size, TOPOLOGY_POLYTOPE);

        while (current != destination && hops < 100) {
            next = thorium_router_get_next_rank_in_route(&router, source,
                            current, destination);

            current = next;
            ++hops;
        }

        TEST_INT_EQUALS(current, destination);
        TEST_INT_EQUALS(hops, 3);

        thorium_router_destroy(&router);
    }

    END_TESTS();

    return 0;
}
