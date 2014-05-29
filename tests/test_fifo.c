
#include <structures/fifo.h>

#include "test.h"

int main(int argc, char **argv)
{
    BEGIN_TESTS();

    {
        /* test when inserting more than array size */

        struct bsal_fifo fifo;
        int i;
        bsal_fifo_init(&fifo, 16, sizeof(int));

        TEST_INT_EQUALS(bsal_fifo_empty(&fifo), 1);

        i = 16;
        while (i--) {
            TEST_INT_EQUALS(bsal_fifo_push(&fifo, &i), 1);
        }

        i = 16;
        while (i--) {
            TEST_INT_EQUALS(bsal_fifo_push(&fifo, &i), 1);
        }

        TEST_INT_EQUALS(bsal_fifo_full(&fifo), 0);

        i = 16;
        while (i--) {
            int item;
            TEST_INT_EQUALS(bsal_fifo_pop(&fifo, &item), 1);
        }

        i = 16;
        while (i--) {
            int item;
            TEST_INT_EQUALS(bsal_fifo_pop(&fifo, &item), 1);
        }

        bsal_fifo_destroy(&fifo);

    }

    {
        /* push 1000 elements, and verify them after */

        struct bsal_fifo fifo;
        int i;
        bsal_fifo_init(&fifo, 10, sizeof(int));

        TEST_INT_EQUALS(bsal_fifo_empty(&fifo), 1);

        i = 1000;
        while (i--) {
            TEST_INT_EQUALS(bsal_fifo_push(&fifo, &i), 1);
        }

        TEST_INT_EQUALS(bsal_fifo_full(&fifo), 0);

        i = 1000;
        while (i--) {
            int item;
            TEST_INT_EQUALS(bsal_fifo_pop(&fifo, &item), 1);
            TEST_INT_EQUALS(item, i);
            /* printf("%i %i\n", item, i); */
        }

        TEST_INT_EQUALS(bsal_fifo_empty(&fifo), 1);
    }

    {
        /* use array size of 1 and 2000 elements */

        struct bsal_fifo fifo;
        int i;
        bsal_fifo_init(&fifo, 1, sizeof(int));

        TEST_INT_EQUALS(bsal_fifo_empty(&fifo), 1);

        i = 2000;
        while (i--) {
            TEST_INT_EQUALS(bsal_fifo_push(&fifo, &i), 1);
        }

        TEST_INT_EQUALS(bsal_fifo_full(&fifo), 0);

        i = 2000;
        while (i--) {
            int item;
            TEST_INT_EQUALS(bsal_fifo_pop(&fifo, &item), 1);
            TEST_INT_EQUALS(item, i);
            /* printf("%i %i\n", item, i); */
        }

        TEST_INT_EQUALS(bsal_fifo_empty(&fifo), 1);
    }

    {
        /* stress test the code by inserting one element,
           and then removing. */

        struct bsal_fifo fifo;
        int i;
        bsal_fifo_init(&fifo, 4, sizeof(int));

        TEST_INT_EQUALS(bsal_fifo_empty(&fifo), 1);

        i = 3000;
        while (i--) {
            TEST_INT_EQUALS(bsal_fifo_push(&fifo, &i), 1);
            TEST_INT_EQUALS(bsal_fifo_pop(&fifo, &i), 1);
            TEST_INT_EQUALS(bsal_fifo_empty(&fifo), 1);
            TEST_INT_EQUALS(bsal_fifo_push(&fifo, &i), 1);
            TEST_INT_EQUALS(bsal_fifo_pop(&fifo, &i), 1);
        }

        TEST_INT_EQUALS(bsal_fifo_full(&fifo), 0);
    }


    END_TESTS();

    return 0;
}
