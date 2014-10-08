
#include "fast_queue_iterator.h"

#include "fast_queue.h"

void core_fast_queue_iterator_init(struct core_fast_queue_iterator *self, struct core_fast_queue *container)
{
    self->container = container;
    self->i = 0;
    self->size =core_fast_queue_size(container);
}

void core_fast_queue_iterator_destroy(struct core_fast_queue_iterator *self)
{
    self->container = NULL;
    self->i = 0;
    self->size = 0;
}

int core_fast_queue_iterator_has_next(struct core_fast_queue_iterator *self)
{
    if (self->i < self->size)
        return 1;

    return 0;
}

int core_fast_queue_iterator_next_value(struct core_fast_queue_iterator *self, void *value)
{
    if (!core_fast_queue_iterator_has_next(self))
        return 0;

    core_fast_queue_dequeue(self->container, value);
    core_fast_queue_enqueue(self->container, value);

    return 1;
}


