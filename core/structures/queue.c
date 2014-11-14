
#include "queue.h"

#include <core/system/memory.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <stdint.h>
#include <inttypes.h>


/*
#define CORE_QUEUE_DEBUG
*/

void core_queue_init(struct core_queue *self, int bytes_per_unit)
{
    self->size = 0;
    self->enqueue_index = 0;
    self->dequeue_index = 0;
    self->bytes_per_element = bytes_per_unit;

    core_vector_init(&self->vector, bytes_per_unit);
}

void core_queue_destroy(struct core_queue *self)
{
    self->size = 0;
    self->enqueue_index = 0;
    self->dequeue_index = 0;
    self->bytes_per_element = 0;

    core_vector_destroy(&self->vector);
}

int core_queue_enqueue(struct core_queue *self, void *item)
{
    int stride;
    int i;
    void *bucket1;
    void *bucket2;
    int offset1;
    int offset2;
    int minimum_stride;

#ifdef CORE_QUEUE_DEBUG
    int value;

    value = *(int *)item;

    printf("DEBUG enqueue %d, size %d\n", value, self->size);
#endif

    /* The vector is full, try to move things on the left.
     */
    if (self->enqueue_index == core_vector_size(&self->vector)) {
        stride = self->dequeue_index;

#ifdef CORE_QUEUE_DEBUG
        printf("dequeue_index %d\n", self->dequeue_index);
#endif

        minimum_stride = core_vector_size(&self->vector) / 8;

        /* move things on the left
         */
        if (stride > 0 && self->size > 0
                        && stride >= minimum_stride) {

            i = 0;
            while (i < self->size) {
                offset1 = self->dequeue_index + i;
                offset2 = self->dequeue_index + i - stride;

                bucket1 = core_vector_at(&self->vector, offset1);
                bucket2 = core_vector_at(&self->vector, offset2);

#ifdef CORE_QUEUE_DEBUG
                printf("DEBUG moving %d %p to %d %p (size %d, vector size %d)\n", offset1, bucket1, offset2, bucket2,
                                self->size, (int)core_vector_size(&self->vector));
#endif

                core_memory_copy(bucket2, bucket1, self->bytes_per_element);

                i++;
            }

            self->enqueue_index -= stride;
            self->dequeue_index -= stride;

            bucket1 = core_vector_at(&self->vector, self->enqueue_index);
            core_memory_copy(bucket1, item, self->bytes_per_element);
            self->enqueue_index++;
            self->size++;
            return 1;
        }

        /* there was no place, append the item in the vector
         */

        core_vector_push_back(&self->vector, item);
        self->enqueue_index++;
        self->size++;

        return 1;
    }

    /* at this point, there is plenty of space...
     */

    bucket1 = core_vector_at(&self->vector, self->enqueue_index);
    core_memory_copy(bucket1, item, self->bytes_per_element);
    self->enqueue_index++;
    self->size++;

    return 1;
}

int core_queue_dequeue(struct core_queue *self, void *item)
{
    void *bucket;
    if (self->size == 0) {
        return 0;
    }

    bucket = core_vector_at(&self->vector, self->dequeue_index);
    core_memory_copy(item, bucket, self->bytes_per_element);
    self->dequeue_index++;
    self->size--;

    return 1;
}

int core_queue_empty(struct core_queue *self)
{
    return self->size == 0;
}

int core_queue_full(struct core_queue *self)
{
    return 0;
}

int core_queue_size(struct core_queue *self)
{
    return self->size;
}

void core_queue_set_memory_pool(struct core_queue *self,
                struct core_memory_pool *pool)
{
    core_vector_set_memory_pool(&self->vector, pool);
}

void core_queue_print(struct core_queue *self)
{
    printf("core_queue_print capacity is %" PRId64 "\n",
                    core_vector_capacity(&self->vector));
}

int core_queue_capacity(struct core_queue *self)
{
    return core_vector_capacity(&self->vector);
}
