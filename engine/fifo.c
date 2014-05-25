
#include "fifo.h"

#include <stdlib.h>

void bsal_fifo_init(struct bsal_fifo *fifo, int units, int bytes_per_unit)
{
    struct bsal_fifo_array *array;

    fifo->units = units;
    fifo->bytes_per_unit = bytes_per_unit;

    /* allocate an initial array
     */
    array = bsal_fifo_get_array(fifo);

    fifo->first = array;
    fifo->last = array;
    fifo->bin = NULL;
}

void bsal_fifo_destroy(struct bsal_fifo *fifo)
{
    bsal_fifo_delete(fifo->first);
    bsal_fifo_delete(fifo->bin);

    fifo->first = NULL;
    fifo->last = NULL;
    fifo->bin = NULL;

    fifo->units = 0;
    fifo->bytes_per_unit = 0;
}

void bsal_fifo_delete(struct bsal_fifo_array *array)
{
    struct bsal_fifo_array *current;
    struct bsal_fifo_array *next;

    current = array;

    while(current != NULL) {
        next = bsal_fifo_array_next(current);
        bsal_fifo_destroy_array(current);
        current = next;
    }
}

void bsal_fifo_destroy_array(struct bsal_fifo_array *array)
{
    bsal_fifo_array_destroy(array);

    /* TODO use a slab allocator */
    free(array);
}

int bsal_fifo_push(struct bsal_fifo *fifo, void *item)
{
    struct bsal_fifo_array *array;

    if (bsal_fifo_array_push(fifo->last, item)) {
        return 1;
    }

    /* add an array and retry... */

    array = bsal_fifo_get_array(fifo);
    bsal_fifo_array_set_previous(array, fifo->last);
    bsal_fifo_array_set_next(fifo->last, array);
    fifo->last = array;

    /* recursive call... */
    /* there should be at most 1 recursive call */
    return bsal_fifo_push(fifo, item);
}

int bsal_fifo_pop(struct bsal_fifo *fifo, void *item)
{
    struct bsal_fifo_array *array;

    if (bsal_fifo_array_pop(fifo->first, item)) {

        /* check if it is empty
         * if it is empty, we remove it from the system
         */
        if (bsal_fifo_array_empty(fifo->first)) {

            /* reset it so that it is ready to receive stuff */
            if (fifo->first == fifo->last) {
                bsal_fifo_array_reset(fifo->first);

                return 1;
            }

            /* otherwise, remove the first array */
            array = fifo->first;

            fifo->first = bsal_fifo_array_next(fifo->first);
            bsal_fifo_array_set_previous(fifo->first, NULL);

            bsal_fifo_destroy_array(array);
        }

        return 1;
    }

    return 0;
}

int bsal_fifo_bytes(struct bsal_fifo *fifo)
{
    return fifo->bytes_per_unit;
}

int bsal_fifo_empty(struct bsal_fifo *fifo)
{
    if (fifo->first == fifo->last) {
        return bsal_fifo_array_empty(fifo->first);
    }

    return 0;
}

int bsal_fifo_full(struct bsal_fifo *fifo)
{
    return 0;
}

struct bsal_fifo_array *bsal_fifo_get_array(struct bsal_fifo *fifo)
{
    struct bsal_fifo_array *array;

    /* TODO: use a slab allocator here */
    array = (struct bsal_fifo_array *)malloc(sizeof(struct bsal_fifo_array));
    bsal_fifo_array_init(array, fifo->units, fifo->bytes_per_unit);

    return array;
}
