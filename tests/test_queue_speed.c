
#include <core/structures/queue.h>

#include "test.h"

int main(int argc, char **argv)
{
    BEGIN_TESTS();

    {
        struct core_queue queue;
        int i;
        int value;
        core_queue_init(&queue, sizeof(int));

        i = 300000000;
        while (i--) {
            value = i;

            core_queue_enqueue(&queue, &value);
            core_queue_enqueue(&queue, &value);
            core_queue_dequeue(&queue, &value);
            core_queue_enqueue(&queue, &value);
            core_queue_enqueue(&queue, &value);
            core_queue_dequeue(&queue, &value);
            core_queue_enqueue(&queue, &value);
            core_queue_enqueue(&queue, &value);
            core_queue_enqueue(&queue, &value);
            core_queue_dequeue(&queue, &value);
            core_queue_dequeue(&queue, &value);
        }

        core_queue_destroy(&queue);
    }


    END_TESTS();

    return 0;
}
