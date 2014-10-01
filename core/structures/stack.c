
#include "stack.h"

#include <core/system/memory.h>

void biosal_stack_init(struct biosal_stack *self, int value_size)
{
    biosal_vector_init(&self->vector, value_size);
}

void biosal_stack_destroy(struct biosal_stack *self)
{
    biosal_vector_destroy(&self->vector);
}

int biosal_stack_pop(struct biosal_stack *self, void *value)
{
    int i;
    int size;
    void *bucket;

    size = biosal_vector_size(&self->vector);

    if (size == 0)
        return 0;

    i = size - 1;
    bucket = biosal_vector_at(&self->vector, i);
    biosal_memory_copy(value, bucket, biosal_vector_element_size(&self->vector));
    biosal_vector_resize(&self->vector, size - 1);

    return 1;
}

int biosal_stack_push(struct biosal_stack *self, void *value)
{
    biosal_vector_push_back(&self->vector, value);

    return 1;
}

int biosal_stack_size(struct biosal_stack *self)
{
    return biosal_vector_size(&self->vector);
}

int biosal_stack_empty(struct biosal_stack *self)
{
    return biosal_stack_size(self) == 0;
}

int biosal_stack_full(struct biosal_stack *self)
{
    return 0;
}
