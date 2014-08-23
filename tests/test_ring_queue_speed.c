
#include <core/structures/fast_queue.h>

#include "test.h"

int main(int argc, char **argv)
{
    BEGIN_TESTS();

    {
        struct bsal_fast_queue queue;
        int i;
        int value;
        bsal_fast_queue_init(&queue, sizeof(int));

        i = 300000000;
        while (i--) {
            value = i;

            bsal_fast_queue_enqueue(&queue, &value);
            bsal_fast_queue_enqueue(&queue, &value);
            bsal_fast_queue_dequeue(&queue, &value);
            bsal_fast_queue_enqueue(&queue, &value);
            bsal_fast_queue_enqueue(&queue, &value);
            bsal_fast_queue_dequeue(&queue, &value);
            bsal_fast_queue_enqueue(&queue, &value);
            bsal_fast_queue_enqueue(&queue, &value);
            bsal_fast_queue_enqueue(&queue, &value);
            bsal_fast_queue_dequeue(&queue, &value);
            bsal_fast_queue_dequeue(&queue, &value);
        }

        bsal_fast_queue_destroy(&queue);
    }


    END_TESTS();

    return 0;
}
