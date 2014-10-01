
#include <core/structures/queue.h>

#include "test.h"

int main(int argc, char **argv)
{
    BEGIN_TESTS();

    {
        struct biosal_queue queue;
        int i;
        int value;
        biosal_queue_init(&queue, sizeof(int));

        i = 300000000;
        while (i--) {
            value = i;

            biosal_queue_enqueue(&queue, &value);
            biosal_queue_enqueue(&queue, &value);
            biosal_queue_dequeue(&queue, &value);
            biosal_queue_enqueue(&queue, &value);
            biosal_queue_enqueue(&queue, &value);
            biosal_queue_dequeue(&queue, &value);
            biosal_queue_enqueue(&queue, &value);
            biosal_queue_enqueue(&queue, &value);
            biosal_queue_enqueue(&queue, &value);
            biosal_queue_dequeue(&queue, &value);
            biosal_queue_dequeue(&queue, &value);
        }

        biosal_queue_destroy(&queue);
    }


    END_TESTS();

    return 0;
}
