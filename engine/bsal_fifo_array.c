
#include "bsal_fifo_array.h"

#include <stdlib.h>
#include <string.h>

void bsal_fifo_array_construct(struct bsal_fifo_array *fifo, int units, int bytes_per_unit)
{
    int bytes;

    fifo->units = units;
    fifo->bytes_per_unit = bytes_per_unit;
    bytes = bsal_fifo_array_bytes(fifo);
    fifo->array = malloc(bytes * sizeof(char));

    bsal_fifo_array_reset(fifo);

    fifo->previous  = NULL;
    fifo->next = NULL;
}

void bsal_fifo_array_destruct(struct bsal_fifo_array *fifo)
{
    fifo->units = 0;
    fifo->bytes_per_unit = 0;
    fifo->array = NULL;
    fifo->consumer_head = -1;
    fifo->producer_tail = -1;
}

void bsal_fifo_array_reset(struct bsal_fifo_array *fifo)
{
    fifo->consumer_head = 0;
    fifo->producer_tail = 0;
}

int bsal_fifo_array_bytes(struct bsal_fifo_array *fifo)
{
    return fifo->units * fifo->bytes_per_unit;
}

int bsal_fifo_array_push(struct bsal_fifo_array *fifo, void *item)
{
    int position;
    void *destination;

    if (bsal_fifo_array_full(fifo)) {
        return 0;
    }

    position = fifo->producer_tail++;
    destination = bsal_fifo_array_unit(fifo, position);
    memcpy(destination, item, fifo->bytes_per_unit);

    return 1;
}

void *bsal_fifo_array_unit(struct bsal_fifo_array *fifo, int position)
{
    return ((char *)fifo->array) + position * fifo->bytes_per_unit;
}

int bsal_fifo_array_pop(struct bsal_fifo_array *fifo, void *item)
{
    int position;
    void *source;

    if (bsal_fifo_array_empty(fifo)) {
        return 0;
    }

    position = fifo->consumer_head++;
    source =  bsal_fifo_array_unit(fifo, position);
    memcpy(item, source, fifo->bytes_per_unit);

    return 1;
}

int bsal_fifo_array_full(struct bsal_fifo_array *fifo)
{
    return fifo->producer_tail == fifo->units;
}

int bsal_fifo_array_empty(struct bsal_fifo_array *fifo)
{
    return fifo->producer_tail <= fifo->consumer_head;
}

void bsal_fifo_array_set_next(struct bsal_fifo_array *fifo, struct bsal_fifo_array *item)
{
    fifo->next = item;
}

void bsal_fifo_array_set_previous(struct bsal_fifo_array *fifo, struct bsal_fifo_array *item)
{
    fifo->previous = item;
}

struct bsal_fifo_array *bsal_fifo_array_next(struct bsal_fifo_array *fifo)
{
    return fifo->next;
}

struct bsal_fifo_array *bsal_fifo_array_previous(struct bsal_fifo_array *fifo)
{
    return fifo->previous;
}
