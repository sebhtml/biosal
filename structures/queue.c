
#include "queue.h"

#include <stdlib.h>

void bsal_queue_init(struct bsal_queue *queue, int bytes_per_unit)
{
    struct bsal_queue_group *array;
    int units;

    units = 64;

    queue->units = units;
    queue->bytes_per_unit = bytes_per_unit;

    /* allocate an initial array
     */
    array = bsal_queue_get_array(queue);

    queue->first = array;
    queue->last = array;
    queue->bin = NULL;
    queue->size = 0;
}

void bsal_queue_destroy(struct bsal_queue *queue)
{
    bsal_queue_delete(queue->first);
    bsal_queue_delete(queue->bin);

    queue->first = NULL;
    queue->last = NULL;
    queue->bin = NULL;

    queue->units = 0;
    queue->bytes_per_unit = 0;
}

void bsal_queue_delete(struct bsal_queue_group *array)
{
    struct bsal_queue_group *current;
    struct bsal_queue_group *next;

    current = array;

    while (current != NULL) {
        next = bsal_queue_group_next(current);
        bsal_queue_destroy_array(current);
        current = next;
    }
}

void bsal_queue_destroy_array(struct bsal_queue_group *array)
{
    bsal_queue_group_destroy(array);

    /* TODO use a slab allocator */
    free(array);
}

int bsal_queue_enqueue(struct bsal_queue *queue, void *item)
{
    struct bsal_queue_group *array;

    if (bsal_queue_group_push(queue->last, item)) {
        queue->size++;
        return 1;
    }

    /* add an array and retry... */

    array = bsal_queue_get_array(queue);
    bsal_queue_group_set_previous(array, queue->last);
    bsal_queue_group_set_next(queue->last, array);
    queue->last = array;

    /* recursive call... */
    /* there should be at most 1 recursive call */
    return bsal_queue_enqueue(queue, item);
}

int bsal_queue_dequeue(struct bsal_queue *queue, void *item)
{
    struct bsal_queue_group *array;

    /* empty. */
    if (bsal_queue_empty(queue)) {
        return 0;
    }

    if (bsal_queue_group_pop(queue->first, item)) {

        /* check if it is empty
         * if it is empty, we remove it from the system
         */
        if (bsal_queue_group_empty(queue->first)) {

            /* reset it so that it is ready to receive stuff */
            if (queue->first == queue->last) {
                bsal_queue_group_reset(queue->first);

                queue->size--;
                return 1;
            }

            /* otherwise, remove the first array */
            array = queue->first;

            queue->first = bsal_queue_group_next(queue->first);
            bsal_queue_group_set_previous(queue->first, NULL);

            bsal_queue_destroy_array(array);
        }

        queue->size--;
        return 1;
    }

    return 0;
}

int bsal_queue_bytes(struct bsal_queue *queue)
{
    return queue->bytes_per_unit;
}

int bsal_queue_empty(struct bsal_queue *queue)
{
    return queue->size == 0;
}

int bsal_queue_size(struct bsal_queue *queue)
{
    return queue->size;
}

int bsal_queue_full(struct bsal_queue *queue)
{
    return 0;
}

struct bsal_queue_group *bsal_queue_get_array(struct bsal_queue *queue)
{
    struct bsal_queue_group *array;

    /* TODO: use a slab allocator here */
    array = (struct bsal_queue_group *)malloc(sizeof(struct bsal_queue_group));
    bsal_queue_group_init(array, queue->units, queue->bytes_per_unit);

    return array;
}
