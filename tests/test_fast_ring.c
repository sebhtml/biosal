
#include <core/structures/fast_ring.h>

#include "test.h"

int main(int argc, char **argv)
{
    BEGIN_TESTS();

    struct biosal_fast_ring ring;
    int capacity = 64;
    int i;
    int value;
    int elements;

    elements = 0;

#if 0
    printf("Required %d\n", capacity);
#endif
    biosal_fast_ring_init(&ring, capacity, sizeof(int));

    capacity = biosal_fast_ring_capacity(&ring);

#if 0
    printf("Provided %d\n", capacity);
#endif

    TEST_BOOLEAN_EQUALS(biosal_fast_ring_is_empty_from_consumer(&ring), 1);

    for (i = 0; i < capacity; i++) {
        TEST_BOOLEAN_EQUALS(biosal_fast_ring_push_from_producer(&ring, &i), 1);
        elements++;

        TEST_INT_EQUALS(biosal_fast_ring_size_from_producer(&ring), elements);
        TEST_INT_EQUALS(biosal_fast_ring_size_from_consumer(&ring), elements);
    }

    TEST_BOOLEAN_EQUALS(biosal_fast_ring_is_full_from_producer(&ring), 1);

    for (i = 0; i < capacity; i++) {
        TEST_BOOLEAN_EQUALS(biosal_fast_ring_pop_from_consumer(&ring, &value), 1);
        elements--;
        TEST_INT_EQUALS(value, i);
        TEST_INT_EQUALS(biosal_fast_ring_size_from_producer(&ring), elements);
        TEST_INT_EQUALS(biosal_fast_ring_size_from_consumer(&ring), elements);
    }
    biosal_fast_ring_destroy(&ring);

    END_TESTS();

    return 0;
}
