
#include <structures/queue_group.h>

#include "test.h"

int main(int argc, char **argv)
{
    BEGIN_TESTS();

    {
        struct bsal_queue_group queue;
        int i;
        bsal_queue_group_init(&queue, 16, sizeof(int));

        i = 16;
        while (i--) {
            TEST_INT_EQUALS(bsal_queue_group_push(&queue, &i), 1);
        }

        i = 16;
        while (i--) {
            TEST_INT_EQUALS(bsal_queue_group_push(&queue, &i), 0);
        }

        i = 16;
        while (i--) {
            int item;
            TEST_INT_EQUALS(bsal_queue_group_pop(&queue, &item), 1);
        }

        i = 16;
        while (i--) {
            int item;
            TEST_INT_EQUALS(bsal_queue_group_pop(&queue, &item), 0);
        }

        bsal_queue_group_destroy(&queue);

    }

    END_TESTS();

    return 0;
}
