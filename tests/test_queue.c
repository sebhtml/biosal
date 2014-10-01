
#include <core/structures/queue.h>

#include "test.h"

int main(int argc, char **argv)
{
    BEGIN_TESTS();

    {
        /* test when inserting more than array size */

        struct biosal_queue queue;
        int i;
        biosal_queue_init(&queue, sizeof(int));

        TEST_INT_EQUALS(biosal_queue_empty(&queue), 1);

        i = 16;
        while (i--) {
            TEST_INT_EQUALS(biosal_queue_enqueue(&queue, &i), 1);
        }

        i = 16;
        while (i--) {
            TEST_INT_EQUALS(biosal_queue_enqueue(&queue, &i), 1);
        }

        TEST_INT_EQUALS(biosal_queue_full(&queue), 0);

        i = 16;
        while (i--) {
            int item;
            TEST_INT_EQUALS(biosal_queue_dequeue(&queue, &item), 1);
        }

        i = 16;
        while (i--) {
            int item;
            TEST_INT_EQUALS(biosal_queue_dequeue(&queue, &item), 1);
        }

        biosal_queue_destroy(&queue);

    }

    {
        /* push 1000 elements, and verify them after */

        struct biosal_queue queue;
        int i;
        biosal_queue_init(&queue, sizeof(int));

        TEST_INT_EQUALS(biosal_queue_empty(&queue), 1);

        i = 1000;
        while (i--) {
            TEST_INT_EQUALS(biosal_queue_enqueue(&queue, &i), 1);
        }

        TEST_INT_EQUALS(biosal_queue_full(&queue), 0);

        i = 1000;
        while (i--) {
            int item;
            TEST_INT_EQUALS(biosal_queue_dequeue(&queue, &item), 1);
            TEST_INT_EQUALS(item, i);
            /* printf("%i %i\n", item, i); */
        }

        TEST_INT_EQUALS(biosal_queue_empty(&queue), 1);
    }

    {
        /* use array size of 1 and 2000 elements */

        struct biosal_queue queue;
        int i;
        biosal_queue_init(&queue, sizeof(int));

        TEST_INT_EQUALS(biosal_queue_empty(&queue), 1);

        i = 2000;
        while (i--) {
            TEST_INT_EQUALS(biosal_queue_enqueue(&queue, &i), 1);
        }

        TEST_INT_EQUALS(biosal_queue_full(&queue), 0);

        i = 2000;
        while (i--) {
            int item;
            TEST_INT_EQUALS(biosal_queue_dequeue(&queue, &item), 1);
            TEST_INT_EQUALS(item, i);
            /* printf("%i %i\n", item, i); */
        }

        TEST_INT_EQUALS(biosal_queue_empty(&queue), 1);
    }

    {
        /* stress test the code by inserting one element,
           and then removing. */

        struct biosal_queue queue;
        int i;
        biosal_queue_init(&queue, sizeof(int));

        TEST_INT_EQUALS(biosal_queue_empty(&queue), 1);

        i = 3000;
        while (i--) {
            TEST_INT_EQUALS(biosal_queue_enqueue(&queue, &i), 1);
            TEST_INT_EQUALS(biosal_queue_dequeue(&queue, &i), 1);
            TEST_INT_EQUALS(biosal_queue_empty(&queue), 1);
            TEST_INT_EQUALS(biosal_queue_enqueue(&queue, &i), 1);
            TEST_INT_EQUALS(biosal_queue_dequeue(&queue, &i), 1);
        }

        TEST_INT_EQUALS(biosal_queue_full(&queue), 0);
    }

    {
        struct biosal_queue queue;
        int i;
        int value;
        biosal_queue_init(&queue, sizeof(int));

        TEST_INT_EQUALS(biosal_queue_empty(&queue), 1);

        i = 3000;
        while (i--) {
            value = i;
            TEST_INT_EQUALS(biosal_queue_enqueue(&queue, &value), 1);
            TEST_INT_EQUALS(biosal_queue_enqueue(&queue, &value), 1);
            TEST_INT_EQUALS(biosal_queue_dequeue(&queue, &value), 1);
        }

    }


    END_TESTS();

    return 0;
}
