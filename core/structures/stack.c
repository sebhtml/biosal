
#include "stack.h"

#include <core/system/memory.h>

void bsal_stack_init(struct bsal_stack *self, int value_size)
{
    bsal_vector_init(&self->vector, value_size);
}

void bsal_stack_destroy(struct bsal_stack *self)
{
    bsal_vector_destroy(&self->vector);
}

int bsal_stack_pop(struct bsal_stack *self, void *value)
{
    int i;
    int size;
    void *bucket;

    size = bsal_vector_size(&self->vector);

    if (size == 0)
        return 0;

    i = size - 1;
    bucket = bsal_vector_at(&self->vector, i);
    bsal_memory_copy(value, bucket, bsal_vector_element_size(&self->vector));
    bsal_vector_resize(&self->vector, size - 1);

    return 1;
}

int bsal_stack_push(struct bsal_stack *self, void *value)
{
    bsal_vector_push_back(&self->vector, value);

    return 1;
}

int bsal_stack_size(struct bsal_stack *self)
{
    return bsal_vector_size(&self->vector);
}

int bsal_stack_empty(struct bsal_stack *self)
{
    return bsal_stack_size(self) == 0;
}

int bsal_stack_full(struct bsal_stack *self)
{
    return 0;
}
