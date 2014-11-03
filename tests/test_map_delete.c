
#include <core/structures/map.h>

#include "test.h"

#include <stdint.h>
#include <inttypes.h>

int main(int argc, char **argv)
{
    BEGIN_TESTS();

    struct core_map container;
    int key_size = sizeof(void *);
    int value_size = sizeof(size_t);
    uint64_t i;
    uint64_t element_count;

    element_count = 100000;

    core_map_init(&container, key_size, value_size);

    for (i = 0; i < element_count; ++i) {

        TEST_POINTER_EQUALS(core_map_get(&container, &i), NULL);
        core_map_add(&container, &i);
        TEST_POINTER_NOT_EQUALS(core_map_get(&container, &i), NULL);

        /*
         * Delete something once in a while
         */
        if (i % 2 == 0) {

            TEST_POINTER_NOT_EQUALS(core_map_get(&container, &i), NULL);
            core_map_delete(&container, &i);
            TEST_POINTER_EQUALS(core_map_get(&container, &i), NULL);

            TEST_POINTER_EQUALS(core_map_get(&container, &i), NULL);
            core_map_add(&container, &i);
            TEST_POINTER_NOT_EQUALS(core_map_get(&container, &i), NULL);
        }
    }

    core_map_destroy(&container);

    END_TESTS();

    return 0;
}
