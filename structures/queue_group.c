
#include "queue_group.h"

#include <stdlib.h>
#include <string.h>

void bsal_queue_group_init(struct bsal_queue_group *queue, int units, int bytes_per_unit)
{
    int bytes;

    queue->units = units;
    queue->bytes_per_unit = bytes_per_unit;
    bytes = bsal_queue_group_bytes(queue);
    queue->array = malloc(bytes * sizeof(char));

    bsal_queue_group_reset(queue);

    queue->previous = NULL;
    queue->next = NULL;
}

void bsal_queue_group_destroy(struct bsal_queue_group *queue)
{
    queue->units = 0;
    queue->bytes_per_unit = 0;
    queue->array = NULL;
    queue->consumer_head = -1;
    queue->producer_tail = -1;
}

void bsal_queue_group_reset(struct bsal_queue_group *queue)
{
    queue->consumer_head = 0;
    queue->producer_tail = 0;
}

int bsal_queue_group_bytes(struct bsal_queue_group *queue)
{
    return queue->units * queue->bytes_per_unit;
}

int bsal_queue_group_push(struct bsal_queue_group *queue, void *item)
{
    int position;
    void *destination;

    if (bsal_queue_group_full(queue)) {
        return 0;
    }

    position = queue->producer_tail++;
    destination = bsal_queue_group_unit(queue, position);
    memcpy(destination, item, queue->bytes_per_unit);

    return 1;
}

void *bsal_queue_group_unit(struct bsal_queue_group *queue, int position)
{
    return ((char *)queue->array) + position * queue->bytes_per_unit;
}

int bsal_queue_group_pop(struct bsal_queue_group *queue, void *item)
{
    int position;
    void *source;

    if (bsal_queue_group_empty(queue)) {
        return 0;
    }

    position = queue->consumer_head++;
    source = bsal_queue_group_unit(queue, position);
    memcpy(item, source, queue->bytes_per_unit);

    return 1;
}

int bsal_queue_group_full(struct bsal_queue_group *queue)
{
    return queue->producer_tail == queue->units;
}

int bsal_queue_group_empty(struct bsal_queue_group *queue)
{
    return queue->producer_tail <= queue->consumer_head;
}

void bsal_queue_group_set_next(struct bsal_queue_group *queue, struct bsal_queue_group *item)
{
    queue->next = item;
}

void bsal_queue_group_set_previous(struct bsal_queue_group *queue, struct bsal_queue_group *item)
{
    queue->previous = item;
}

struct bsal_queue_group *bsal_queue_group_next(struct bsal_queue_group *queue)
{
    return queue->next;
}

struct bsal_queue_group *bsal_queue_group_previous(struct bsal_queue_group *queue)
{
    return queue->previous;
}
