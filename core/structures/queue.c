
#include "queue.h"

#include <core/system/memory.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
#define BSAL_QUEUE_DEBUG
*/

void bsal_queue_init(struct bsal_queue *self, int bytes_per_unit)
{
    self->size = 0;
    self->enqueue_index = 0;
    self->dequeue_index = 0;
    self->bytes_per_element = bytes_per_unit;

    bsal_vector_init(&self->vector, bytes_per_unit);
}

void bsal_queue_destroy(struct bsal_queue *self)
{
    self->size = 0;
    self->enqueue_index = 0;
    self->dequeue_index = 0;
    self->bytes_per_element = 0;

    bsal_vector_destroy(&self->vector);
}

int bsal_queue_enqueue(struct bsal_queue *self, void *item)
{
    int stride;
    int i;
    void *bucket1;
    void *bucket2;
    int offset1;
    int offset2;
    int minimum_stride;

#ifdef BSAL_QUEUE_DEBUG
    int value;

    value = *(int *)item;

    printf("DEBUG enqueue %d, size %d\n", value, self->size);
#endif

    /* The vector is full, try to move things on the left.
     */
    if (self->enqueue_index == bsal_vector_size(&self->vector)) {
        stride = self->dequeue_index;

#ifdef BSAL_QUEUE_DEBUG
        printf("dequeue_index %d\n", self->dequeue_index);
#endif

        minimum_stride = bsal_vector_size(&self->vector) / 8;

        /* move things on the left
         */
        if (stride > 0 && self->size > 0
                        && stride >= minimum_stride) {

            i = 0;
            while (i < self->size) {
                offset1 = self->dequeue_index + i;
                offset2 = self->dequeue_index + i - stride;

                bucket1 = bsal_vector_at(&self->vector, offset1);
                bucket2 = bsal_vector_at(&self->vector, offset2);

#ifdef BSAL_QUEUE_DEBUG
                printf("DEBUG moving %d %p to %d %p (size %d, vector size %d)\n", offset1, bucket1, offset2, bucket2,
                                self->size, (int)bsal_vector_size(&self->vector));
#endif

                memcpy(bucket2, bucket1, self->bytes_per_element);

                i++;
            }

            self->enqueue_index -= stride;
            self->dequeue_index -= stride;

            bucket1 = bsal_vector_at(&self->vector, self->enqueue_index);
            memcpy(bucket1, item, self->bytes_per_element);
            self->enqueue_index++;
            self->size++;
            return 1;
        }

        /* there was no place, append the item in the vector
         */

        bsal_vector_push_back(&self->vector, item);
        self->enqueue_index++;
        self->size++;

        return 1;
    }

    /* at this point, there is plenty of space...
     */

    bucket1 = bsal_vector_at(&self->vector, self->enqueue_index);
    memcpy(bucket1, item, self->bytes_per_element);
    self->enqueue_index++;
    self->size++;

    return 1;
}

int bsal_queue_dequeue(struct bsal_queue *self, void *item)
{
    void *bucket;
    if (self->size == 0) {
        return 0;
    }

    bucket = bsal_vector_at(&self->vector, self->dequeue_index);
    memcpy(item, bucket, self->bytes_per_element);
    self->dequeue_index++;
    self->size--;

    return 1;
}

int bsal_queue_empty(struct bsal_queue *self)
{
    return self->size == 0;
}

int bsal_queue_full(struct bsal_queue *self)
{
    return 0;
}

int bsal_queue_size(struct bsal_queue *self)
{
    return self->size;
}


