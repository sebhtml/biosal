
#include "queue.h"

#include <core/system/memory.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
#define BIOSAL_QUEUE_DEBUG
*/

void biosal_queue_init(struct biosal_queue *self, int bytes_per_unit)
{
    self->size = 0;
    self->enqueue_index = 0;
    self->dequeue_index = 0;
    self->bytes_per_element = bytes_per_unit;

    biosal_vector_init(&self->vector, bytes_per_unit);
}

void biosal_queue_destroy(struct biosal_queue *self)
{
    self->size = 0;
    self->enqueue_index = 0;
    self->dequeue_index = 0;
    self->bytes_per_element = 0;

    biosal_vector_destroy(&self->vector);
}

int biosal_queue_enqueue(struct biosal_queue *self, void *item)
{
    int stride;
    int i;
    void *bucket1;
    void *bucket2;
    int offset1;
    int offset2;
    int minimum_stride;

#ifdef BIOSAL_QUEUE_DEBUG
    int value;

    value = *(int *)item;

    printf("DEBUG enqueue %d, size %d\n", value, self->size);
#endif

    /* The vector is full, try to move things on the left.
     */
    if (self->enqueue_index == biosal_vector_size(&self->vector)) {
        stride = self->dequeue_index;

#ifdef BIOSAL_QUEUE_DEBUG
        printf("dequeue_index %d\n", self->dequeue_index);
#endif

        minimum_stride = biosal_vector_size(&self->vector) / 8;

        /* move things on the left
         */
        if (stride > 0 && self->size > 0
                        && stride >= minimum_stride) {

            i = 0;
            while (i < self->size) {
                offset1 = self->dequeue_index + i;
                offset2 = self->dequeue_index + i - stride;

                bucket1 = biosal_vector_at(&self->vector, offset1);
                bucket2 = biosal_vector_at(&self->vector, offset2);

#ifdef BIOSAL_QUEUE_DEBUG
                printf("DEBUG moving %d %p to %d %p (size %d, vector size %d)\n", offset1, bucket1, offset2, bucket2,
                                self->size, (int)biosal_vector_size(&self->vector));
#endif

                biosal_memory_copy(bucket2, bucket1, self->bytes_per_element);

                i++;
            }

            self->enqueue_index -= stride;
            self->dequeue_index -= stride;

            bucket1 = biosal_vector_at(&self->vector, self->enqueue_index);
            biosal_memory_copy(bucket1, item, self->bytes_per_element);
            self->enqueue_index++;
            self->size++;
            return 1;
        }

        /* there was no place, append the item in the vector
         */

        biosal_vector_push_back(&self->vector, item);
        self->enqueue_index++;
        self->size++;

        return 1;
    }

    /* at this point, there is plenty of space...
     */

    bucket1 = biosal_vector_at(&self->vector, self->enqueue_index);
    biosal_memory_copy(bucket1, item, self->bytes_per_element);
    self->enqueue_index++;
    self->size++;

    return 1;
}

int biosal_queue_dequeue(struct biosal_queue *self, void *item)
{
    void *bucket;
    if (self->size == 0) {
        return 0;
    }

    bucket = biosal_vector_at(&self->vector, self->dequeue_index);
    biosal_memory_copy(item, bucket, self->bytes_per_element);
    self->dequeue_index++;
    self->size--;

    return 1;
}

int biosal_queue_empty(struct biosal_queue *self)
{
    return self->size == 0;
}

int biosal_queue_full(struct biosal_queue *self)
{
    return 0;
}

int biosal_queue_size(struct biosal_queue *self)
{
    return self->size;
}


