
#include <core/structures/queue.h>

#include "test.h"

int main(int argc, char **argv)
{
    BEGIN_TESTS();

    {
        /* test when inserting more than array size */

        struct core_queue queue;
        int i;
        core_queue_init(&queue, sizeof(int));

        TEST_INT_EQUALS(core_queue_empty(&queue), 1);

        i = 16;
        while (i--) {
            TEST_INT_EQUALS(core_queue_enqueue(&queue, &i), 1);
        }

        i = 16;
        while (i--) {
            TEST_INT_EQUALS(core_queue_enqueue(&queue, &i), 1);
        }

        TEST_INT_EQUALS(core_queue_full(&queue), 0);

        i = 16;
        while (i--) {
            int item;
            TEST_INT_EQUALS(core_queue_dequeue(&queue, &item), 1);
        }

        i = 16;
        while (i--) {
            int item;
            TEST_INT_EQUALS(core_queue_dequeue(&queue, &item), 1);
        }

        core_queue_destroy(&queue);
    }

    {
        /* push 1000 elements, and verify them after */

        struct core_queue queue;
        int i;
        core_queue_init(&queue, sizeof(int));

        TEST_INT_EQUALS(core_queue_empty(&queue), 1);

        i = 1000;
        while (i--) {
            TEST_INT_EQUALS(core_queue_enqueue(&queue, &i), 1);
        }

        TEST_INT_EQUALS(core_queue_full(&queue), 0);

        i = 1000;
        while (i--) {
            int item;
            TEST_INT_EQUALS(core_queue_dequeue(&queue, &item), 1);
            TEST_INT_EQUALS(item, i);
            /* printf("%i %i\n", item, i); */
        }

        TEST_INT_EQUALS(core_queue_empty(&queue), 1);

        core_queue_destroy(&queue);
    }

    {
        /* use array size of 1 and 2000 elements */

        struct core_queue queue;
        int i;
        core_queue_init(&queue, sizeof(int));

        TEST_INT_EQUALS(core_queue_empty(&queue), 1);

        i = 2000;
        while (i--) {
            TEST_INT_EQUALS(core_queue_enqueue(&queue, &i), 1);
        }

        TEST_INT_EQUALS(core_queue_full(&queue), 0);

        i = 2000;
        while (i--) {
            int item;
            TEST_INT_EQUALS(core_queue_dequeue(&queue, &item), 1);
            TEST_INT_EQUALS(item, i);
            /* printf("%i %i\n", item, i); */
        }

        TEST_INT_EQUALS(core_queue_empty(&queue), 1);

        core_queue_destroy(&queue);
    }

    {
        /* stress test the code by inserting one element,
           and then removing. */

        struct core_queue queue;
        int i;
        int expected;
        core_queue_init(&queue, sizeof(int));

        TEST_INT_EQUALS(core_queue_empty(&queue), 1);

        i = 3000;
        while (i--) {
            expected = i;

            TEST_INT_EQUALS(core_queue_enqueue(&queue, &i), 1);
            TEST_INT_EQUALS(core_queue_dequeue(&queue, &i), 1);

            TEST_INT_EQUALS(i, expected);

            TEST_INT_EQUALS(core_queue_empty(&queue), 1);
            TEST_INT_EQUALS(core_queue_enqueue(&queue, &i), 1);
            TEST_INT_EQUALS(core_queue_dequeue(&queue, &i), 1);

            TEST_INT_EQUALS(i, expected);
        }

        TEST_INT_EQUALS(core_queue_full(&queue), 0);
        TEST_INT_EQUALS(core_queue_empty(&queue), 1);

        /*
         * At any time, there is 0 or 1 elements in the queue.
         */
        /*
        core_queue_print(&queue);
        */

        TEST_INT_IS_LOWER_THAN(core_queue_capacity(&queue), 100);

        core_queue_destroy(&queue);
    }

    {
        struct core_queue queue;
        int i;
        int value;
        core_queue_init(&queue, sizeof(int));

        TEST_INT_EQUALS(core_queue_empty(&queue), 1);

        i = 3000;
        while (i--) {
            value = i;
            TEST_INT_EQUALS(core_queue_enqueue(&queue, &value), 1);
            TEST_INT_EQUALS(core_queue_enqueue(&queue, &value), 1);
            TEST_INT_EQUALS(core_queue_dequeue(&queue, &value), 1);
        }

        core_queue_destroy(&queue);
    }

    END_TESTS();

    return 0;
}
