
#include <core/structures/fast_ring.h>

#include <engine/thorium/message.h>

#include "test.h"

int main(int argc, char **argv)
{
    BEGIN_TESTS();

    struct core_fast_ring ring;
    int capacity = 64;
    int i;
    int value;
    int elements;

    elements = 0;

#if 0
    printf("Required %d\n", capacity);
#endif
    core_fast_ring_init(&ring, capacity, sizeof(int));

    capacity = core_fast_ring_capacity(&ring);

#if 0
    printf("Provided %d\n", capacity);
#endif

    TEST_BOOLEAN_EQUALS(core_fast_ring_is_empty_from_consumer(&ring), 1);

    for (i = 0; i < capacity; i++) {
        TEST_BOOLEAN_EQUALS(core_fast_ring_push_from_producer(&ring, &i), 1);
        elements++;

        TEST_INT_EQUALS(core_fast_ring_size_from_producer(&ring), elements);
        TEST_INT_EQUALS(core_fast_ring_size_from_consumer(&ring), elements);
    }

    TEST_BOOLEAN_EQUALS(core_fast_ring_is_full_from_producer(&ring), 1);

    for (i = 0; i < capacity; i++) {
        TEST_BOOLEAN_EQUALS(core_fast_ring_pop_from_consumer(&ring, &value), 1);
        elements--;
        TEST_INT_EQUALS(value, i);
        TEST_INT_EQUALS(core_fast_ring_size_from_producer(&ring), elements);
        TEST_INT_EQUALS(core_fast_ring_size_from_consumer(&ring), elements);
    }
    core_fast_ring_destroy(&ring);

    {
        struct core_fast_ring ring;
        int capacity = 64;
        struct thorium_message message;
        int action;
        int count;
        void *buffer;
        int inserted;
        int pulled;

        action = 1234;
        count = 0;
        buffer = NULL;

        thorium_message_init(&message, action, count, buffer);

        core_fast_ring_init(&ring, capacity, sizeof(struct thorium_message));
        core_fast_ring_use_multiple_producers(&ring);

        inserted = 0;

        while (core_fast_ring_push_compare_and_swap(&ring, &message, 0)) {
            ++inserted;
        }

        /*
         * 64 + 1 = 65, 128 is the next power of 2 for the mask.
         */
        /*
        TEST_INT_EQUALS(inserted, capacity);
        */

        pulled = 0;

        while (core_fast_ring_pop_and_contend(&ring, &message)) {
            ++pulled;
        }

        TEST_INT_EQUALS(inserted, pulled);

        core_fast_ring_destroy(&ring);

        thorium_message_destroy(&message);
    }

    END_TESTS();

    return 0;
}
