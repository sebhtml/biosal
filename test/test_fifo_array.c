
#include <engine/fifo_array.h>

#include "test.h"

int main(int argc, char **argv)
{
    BEGIN_TESTS();

    {
        struct bsal_fifo_array fifo;
        int i;
        bsal_fifo_array_init(&fifo, 16, sizeof(int));

        i = 16;
        while(i--) {
            TEST_EQUAL(bsal_fifo_array_push(&fifo, &i), 1);
        }

        i = 16;
        while(i--) {
            TEST_EQUAL(bsal_fifo_array_push(&fifo, &i), 0);
        }

        i = 16;
        while(i--) {
            int item;
            TEST_EQUAL(bsal_fifo_array_pop(&fifo, &item), 1);
        }

        i = 16;
        while(i--) {
            int item;
            TEST_EQUAL(bsal_fifo_array_pop(&fifo, &item), 0);
        }

        bsal_fifo_array_destroy(&fifo);

    }

    END_TESTS();

    return 0;
}
