
#include <core/structures/fast_queue.h>

#include "test.h"

int main(int argc, char **argv)
{
    BEGIN_TESTS();

    {
        /* test when inserting more than array size */

        struct core_fast_queue fast_queue;
        int i;
        core_fast_queue_init(&fast_queue, sizeof(int));
        int size;

        size = 0;

        TEST_INT_EQUALS(core_fast_queue_empty(&fast_queue), 1);

        i = 16;
        while (i--) {
            TEST_INT_EQUALS(core_fast_queue_enqueue(&fast_queue, &i), 1);
            ++size;
            TEST_INT_EQUALS(core_fast_queue_size(&fast_queue), size);
        }

        i = 16;
        while (i--) {
            TEST_INT_EQUALS(core_fast_queue_enqueue(&fast_queue, &i), 1);
            ++size;
            TEST_INT_EQUALS(core_fast_queue_size(&fast_queue), size);
        }

        TEST_INT_EQUALS(core_fast_queue_full(&fast_queue), 0);

        i = 16;
        while (i--) {
            int item;
            TEST_INT_EQUALS(core_fast_queue_dequeue(&fast_queue, &item), 1);
            --size;
            TEST_INT_EQUALS(core_fast_queue_size(&fast_queue), size);
        }

        i = 16;
        while (i--) {
            int item;
            TEST_INT_EQUALS(core_fast_queue_dequeue(&fast_queue, &item), 1);
            --size;
            TEST_INT_EQUALS(core_fast_queue_size(&fast_queue), size);
        }

        core_fast_queue_destroy(&fast_queue);

    }

    {
        /* push 1000 elements, and verify them after */

        struct core_fast_queue fast_queue;
        int i;
        int size;

        size = 0;
        core_fast_queue_init(&fast_queue, sizeof(int));

        TEST_INT_EQUALS(core_fast_queue_empty(&fast_queue), 1);

        i = 1000;
        while (i--) {
            TEST_INT_EQUALS(core_fast_queue_enqueue(&fast_queue, &i), 1);
            ++size;
            TEST_INT_EQUALS(core_fast_queue_size(&fast_queue), size);
        }

        TEST_INT_EQUALS(core_fast_queue_full(&fast_queue), 0);

        i = 1000;
        while (i--) {
            int item;
            TEST_INT_EQUALS(core_fast_queue_dequeue(&fast_queue, &item), 1);
            --size;
            TEST_INT_EQUALS(core_fast_queue_size(&fast_queue), size);
            TEST_INT_EQUALS(item, i);
            /* printf("%i %i\n", item, i); */
        }

        TEST_INT_EQUALS(core_fast_queue_empty(&fast_queue), 1);
        core_fast_queue_destroy(&fast_queue);
    }

    {
        /* use array size of 1 and 2000 elements */

        struct core_fast_queue fast_queue;
        int i;
        core_fast_queue_init(&fast_queue, sizeof(int));

        TEST_INT_EQUALS(core_fast_queue_empty(&fast_queue), 1);

        i = 2000;
        while (i--) {
            TEST_INT_EQUALS(core_fast_queue_enqueue(&fast_queue, &i), 1);
        }

        TEST_INT_EQUALS(core_fast_queue_full(&fast_queue), 0);

        i = 2000;
        while (i--) {
            int item;
            TEST_INT_EQUALS(core_fast_queue_dequeue(&fast_queue, &item), 1);
            TEST_INT_EQUALS(item, i);
            /* printf("%i %i\n", item, i); */
        }

        TEST_INT_EQUALS(core_fast_queue_empty(&fast_queue), 1);
        core_fast_queue_destroy(&fast_queue);
    }

    {
        /* stress test the code by inserting one element,
           and then removing. */

        struct core_fast_queue fast_queue;
        int i;
        core_fast_queue_init(&fast_queue, sizeof(int));

        TEST_INT_EQUALS(core_fast_queue_empty(&fast_queue), 1);

        i = 3000;
        while (i--) {
            TEST_INT_EQUALS(core_fast_queue_enqueue(&fast_queue, &i), 1);
            TEST_INT_EQUALS(core_fast_queue_dequeue(&fast_queue, &i), 1);
            TEST_INT_EQUALS(core_fast_queue_empty(&fast_queue), 1);
            TEST_INT_EQUALS(core_fast_queue_enqueue(&fast_queue, &i), 1);
            TEST_INT_EQUALS(core_fast_queue_dequeue(&fast_queue, &i), 1);
        }

        TEST_INT_EQUALS(core_fast_queue_full(&fast_queue), 0);
        core_fast_queue_destroy(&fast_queue);
    }

    {
        struct core_fast_queue fast_queue;
        int i;
        int j;
        int value;
        core_fast_queue_init(&fast_queue, sizeof(int));

        TEST_INT_EQUALS(core_fast_queue_empty(&fast_queue), 1);

        for (j = 0; j < 4; j++) {
            i = 3000;
            while (i--) {
                value = i;
                TEST_INT_EQUALS(core_fast_queue_enqueue(&fast_queue, &value), 1);
                TEST_INT_EQUALS(core_fast_queue_enqueue(&fast_queue, &value), 1);
                TEST_INT_EQUALS(core_fast_queue_dequeue(&fast_queue, &value), 1);
            }

            while (core_fast_queue_dequeue(&fast_queue, &value)) {

            }
        }

        core_fast_queue_destroy(&fast_queue);
    }


    END_TESTS();

    return 0;
}
