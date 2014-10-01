
#include "vector_iterator.h"

#include <core/system/memory.h>

#include <stdlib.h>
#include <string.h>

void biosal_vector_iterator_init(struct biosal_vector_iterator *self, struct biosal_vector *list)
{
    self->list = list;
    self->index = 0;
}

void biosal_vector_iterator_destroy(struct biosal_vector_iterator *self)
{
    self->list = NULL;
    self->index = 0;
}

int biosal_vector_iterator_has_next(struct biosal_vector_iterator *self)
{
    return self->index < biosal_vector_size(self->list);
}

int biosal_vector_iterator_next(struct biosal_vector_iterator *self, void **value)
{
    if (!biosal_vector_iterator_has_next(self)) {
        return 0;
    }

    if (value != NULL) {
        *value = biosal_vector_at(self->list, self->index);
    }

    self->index++;

    return 1;
}

int biosal_vector_iterator_get_next_value(struct biosal_vector_iterator *self, void *value)
{
    void *bucket;
    int size;

    if (!biosal_vector_iterator_next(self, (void **)&bucket)) {
        return 0;
    }

    if (value != NULL) {
        size = biosal_vector_element_size(self->list);
        biosal_memory_copy(value, bucket, size);
    }

    return 1;
}
