
#include "stack.h"

#include <core/system/memory.h>

void core_stack_init(struct core_stack *self, int value_size)
{
    core_vector_init(&self->vector, value_size);
}

void core_stack_destroy(struct core_stack *self)
{
    core_vector_destroy(&self->vector);
}

int core_stack_pop(struct core_stack *self, void *value)
{
    int i;
    int size;
    void *bucket;

    size = core_vector_size(&self->vector);

    if (size == 0)
        return 0;

    i = size - 1;
    bucket = core_vector_at(&self->vector, i);
    core_memory_copy(value, bucket, core_vector_element_size(&self->vector));
    core_vector_resize(&self->vector, size - 1);

    return 1;
}

int core_stack_push(struct core_stack *self, void *value)
{
    core_vector_push_back(&self->vector, value);

    return 1;
}

int core_stack_size(struct core_stack *self)
{
    return core_vector_size(&self->vector);
}

int core_stack_empty(struct core_stack *self)
{
    return core_stack_size(self) == 0;
}

int core_stack_full(struct core_stack *self)
{
    return 0;
}
