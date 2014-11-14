
#include "queue.h"

#include <core/system/memory.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <stdint.h>
#include <inttypes.h>


/*
*/
#define CORE_QUEUE_DEBUG_COMPACT

void core_queue_compact(struct core_queue *self);

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
    void *bucket1;

#ifdef CORE_QUEUE_DEBUG
    int value;

    value = *(int *)item;

    printf("DEBUG enqueue %d, size %d\n", value, self->size);
#endif

    core_queue_compact(self);

    /* There was no place, append the item in the vector
     */
    if (self->enqueue_index == core_vector_size(&self->vector)) {

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

    core_queue_compact(self);

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
    printf("core_queue_print size %d dequeue_index %d enqueue_index %d vector_capacity %" PRId64 "\n",
                    self->size, self->dequeue_index, self->enqueue_index,
                    core_vector_capacity(&self->vector));
}

int core_queue_capacity(struct core_queue *self)
{
    return core_vector_capacity(&self->vector);
}

void core_queue_compact(struct core_queue *self)
{
    int available;
    int minimum;
    int i;
    int index;
    void *source;
    void *destination;

    available = self->dequeue_index;
    minimum = 64;

    /*
    printf("core_queue_compact size %d dequeue_index %d enqueue_index %d\n",
                    self->size, self->dequeue_index, self->enqueue_index);
                    */

    if (available < minimum)
        return;

    i = 0;
    /*
     * move all items.
     */
    while (i < self->size) {
        index = self->dequeue_index + i;

        source = core_vector_at(&self->vector, index);
        destination = core_vector_at(&self->vector, i);

        core_memory_copy(destination, source, self->bytes_per_element);

        ++i;
    }

    self->dequeue_index -= available;
    self->enqueue_index -= available;
}
