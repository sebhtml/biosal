
#include <core/structures/ring.h>

#include "test.h"

int main(int argc, char **argv)
{
    BEGIN_TESTS();

    struct biosal_ring ring;
    int capacity = 64;
    int i;
    int value;

    biosal_ring_init(&ring, capacity, sizeof(int));

    TEST_BOOLEAN_EQUALS(biosal_ring_is_empty(&ring), 1);

    for (i = 0; i < capacity; i++) {
        TEST_BOOLEAN_EQUALS(biosal_ring_push(&ring, &i), 1);
        TEST_INT_EQUALS(biosal_ring_size(&ring), i + 1);
    }

    for (i = 0; i < capacity; i++) {
        TEST_BOOLEAN_EQUALS(biosal_ring_pop(&ring, &value), 1);
        TEST_INT_EQUALS(value, i);
    }
    biosal_ring_destroy(&ring);

    END_TESTS();

    return 0;
}
