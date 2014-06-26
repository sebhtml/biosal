
#include <structures/queue.h>

#include "test.h"

int main(int argc, char **argv)
{
    BEGIN_TESTS();

    {
        struct bsal_queue queue;
        int i;
        int value;
        bsal_queue_init(&queue, sizeof(int));

        i = 300000000;
        while (i--) {
            value = i;

            bsal_queue_enqueue(&queue, &value);
            bsal_queue_enqueue(&queue, &value);
            bsal_queue_dequeue(&queue, &value);
            bsal_queue_enqueue(&queue, &value);
            bsal_queue_enqueue(&queue, &value);
            bsal_queue_dequeue(&queue, &value);
            bsal_queue_enqueue(&queue, &value);
            bsal_queue_enqueue(&queue, &value);
            bsal_queue_enqueue(&queue, &value);
            bsal_queue_dequeue(&queue, &value);
            bsal_queue_dequeue(&queue, &value);
        }

        bsal_queue_destroy(&queue);
    }


    END_TESTS();

    return 0;
}
