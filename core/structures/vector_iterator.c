
#include "vector_iterator.h"

#include <core/system/memory.h>

#include <stdlib.h>
#include <string.h>

void core_vector_iterator_init(struct core_vector_iterator *self, struct core_vector *list)
{
    self->list = list;
    self->index = 0;
}

void core_vector_iterator_destroy(struct core_vector_iterator *self)
{
    self->list = NULL;
    self->index = 0;
}

int core_vector_iterator_has_next(struct core_vector_iterator *self)
{
    return self->index < core_vector_size(self->list);
}

int core_vector_iterator_next(struct core_vector_iterator *self, void **value)
{
    if (!core_vector_iterator_has_next(self)) {
        return 0;
    }

    if (value != NULL) {
        *value = core_vector_at(self->list, self->index);
    }

    self->index++;

    return 1;
}

int core_vector_iterator_get_next_value(struct core_vector_iterator *self, void *value)
{
    void *bucket;
    int size;

    if (!core_vector_iterator_next(self, (void **)&bucket)) {
        return 0;
    }

    if (value != NULL) {
        size = core_vector_element_size(self->list);
        core_memory_copy(value, bucket, size);
    }

    return 1;
}
