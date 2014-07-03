
#include <core/structures/ring.h>

#include "test.h"

int main(int argc, char **argv)
{
    BEGIN_TESTS();

    struct bsal_ring ring;
    int capacity = 64;
    int i;
    int value;

    bsal_ring_init(&ring, capacity, sizeof(int));

    TEST_BOOLEAN_EQUALS(bsal_ring_is_empty(&ring), 1);

    for (i = 0; i < capacity; i++) {
        TEST_BOOLEAN_EQUALS(bsal_ring_push(&ring, &i), 1);
        TEST_INT_EQUALS(bsal_ring_size(&ring), i + 1);
    }

    for (i = 0; i < capacity; i++) {
        TEST_BOOLEAN_EQUALS(bsal_ring_pop(&ring, &value), 1);
        TEST_INT_EQUALS(value, i);
    }
    bsal_ring_destroy(&ring);

    END_TESTS();

    return 0;
}
