
#include <core/structures/ring_queue.h>

#include "test.h"

int main(int argc, char **argv)
{
    BEGIN_TESTS();

    {
        /* test when inserting more than array size */

        struct bsal_ring_queue ring_queue;
        int i;
        bsal_ring_queue_init(&ring_queue, sizeof(int));
        int size;

        size = 0;

        TEST_INT_EQUALS(bsal_ring_queue_empty(&ring_queue), 1);

        i = 16;
        while (i--) {
            TEST_INT_EQUALS(bsal_ring_queue_enqueue(&ring_queue, &i), 1);
            ++size;
            TEST_INT_EQUALS(bsal_ring_queue_size(&ring_queue), size);
        }

        i = 16;
        while (i--) {
            TEST_INT_EQUALS(bsal_ring_queue_enqueue(&ring_queue, &i), 1);
            ++size;
            TEST_INT_EQUALS(bsal_ring_queue_size(&ring_queue), size);
        }

        TEST_INT_EQUALS(bsal_ring_queue_full(&ring_queue), 0);

        i = 16;
        while (i--) {
            int item;
            TEST_INT_EQUALS(bsal_ring_queue_dequeue(&ring_queue, &item), 1);
            --size;
            TEST_INT_EQUALS(bsal_ring_queue_size(&ring_queue), size);
        }

        i = 16;
        while (i--) {
            int item;
            TEST_INT_EQUALS(bsal_ring_queue_dequeue(&ring_queue, &item), 1);
            --size;
            TEST_INT_EQUALS(bsal_ring_queue_size(&ring_queue), size);
        }

        bsal_ring_queue_destroy(&ring_queue);

    }

    {
        /* push 1000 elements, and verify them after */

        struct bsal_ring_queue ring_queue;
        int i;
        int size;

        size = 0;
        bsal_ring_queue_init(&ring_queue, sizeof(int));

        TEST_INT_EQUALS(bsal_ring_queue_empty(&ring_queue), 1);

        i = 1000;
        while (i--) {
            TEST_INT_EQUALS(bsal_ring_queue_enqueue(&ring_queue, &i), 1);
            ++size;
            TEST_INT_EQUALS(bsal_ring_queue_size(&ring_queue), size);
        }

        TEST_INT_EQUALS(bsal_ring_queue_full(&ring_queue), 0);

        i = 1000;
        while (i--) {
            int item;
            TEST_INT_EQUALS(bsal_ring_queue_dequeue(&ring_queue, &item), 1);
            --size;
            TEST_INT_EQUALS(bsal_ring_queue_size(&ring_queue), size);
            TEST_INT_EQUALS(item, i);
            /* printf("%i %i\n", item, i); */
        }

        TEST_INT_EQUALS(bsal_ring_queue_empty(&ring_queue), 1);
    }

    {
        /* use array size of 1 and 2000 elements */

        struct bsal_ring_queue ring_queue;
        int i;
        bsal_ring_queue_init(&ring_queue, sizeof(int));

        TEST_INT_EQUALS(bsal_ring_queue_empty(&ring_queue), 1);

        i = 2000;
        while (i--) {
            TEST_INT_EQUALS(bsal_ring_queue_enqueue(&ring_queue, &i), 1);
        }

        TEST_INT_EQUALS(bsal_ring_queue_full(&ring_queue), 0);

        i = 2000;
        while (i--) {
            int item;
            TEST_INT_EQUALS(bsal_ring_queue_dequeue(&ring_queue, &item), 1);
            TEST_INT_EQUALS(item, i);
            /* printf("%i %i\n", item, i); */
        }

        TEST_INT_EQUALS(bsal_ring_queue_empty(&ring_queue), 1);
    }

    {
        /* stress test the code by inserting one element,
           and then removing. */

        struct bsal_ring_queue ring_queue;
        int i;
        bsal_ring_queue_init(&ring_queue, sizeof(int));

        TEST_INT_EQUALS(bsal_ring_queue_empty(&ring_queue), 1);

        i = 3000;
        while (i--) {
            TEST_INT_EQUALS(bsal_ring_queue_enqueue(&ring_queue, &i), 1);
            TEST_INT_EQUALS(bsal_ring_queue_dequeue(&ring_queue, &i), 1);
            TEST_INT_EQUALS(bsal_ring_queue_empty(&ring_queue), 1);
            TEST_INT_EQUALS(bsal_ring_queue_enqueue(&ring_queue, &i), 1);
            TEST_INT_EQUALS(bsal_ring_queue_dequeue(&ring_queue, &i), 1);
        }

        TEST_INT_EQUALS(bsal_ring_queue_full(&ring_queue), 0);
    }

    {
        struct bsal_ring_queue ring_queue;
        int i;
        int value;
        bsal_ring_queue_init(&ring_queue, sizeof(int));

        TEST_INT_EQUALS(bsal_ring_queue_empty(&ring_queue), 1);

        i = 3000;
        while (i--) {
            value = i;
            TEST_INT_EQUALS(bsal_ring_queue_enqueue(&ring_queue, &value), 1);
            TEST_INT_EQUALS(bsal_ring_queue_enqueue(&ring_queue, &value), 1);
            TEST_INT_EQUALS(bsal_ring_queue_dequeue(&ring_queue, &value), 1);
        }

    }


    END_TESTS();

    return 0;
}
