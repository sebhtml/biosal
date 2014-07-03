
#include "vector_iterator.h"

#include <stdlib.h>
#include <string.h>

void bsal_vector_iterator_init(struct bsal_vector_iterator *self, struct bsal_vector *list)
{
    self->list = list;
    self->index = 0;
}

void bsal_vector_iterator_destroy(struct bsal_vector_iterator *self)
{
    self->list = NULL;
    self->index = 0;
}

int bsal_vector_iterator_has_next(struct bsal_vector_iterator *self)
{
    return self->index < bsal_vector_size(self->list);
}

int bsal_vector_iterator_next(struct bsal_vector_iterator *self, void **value)
{
    if (!bsal_vector_iterator_has_next(self)) {
        return 0;
    }

    if (value != NULL) {
        *value = bsal_vector_at(self->list, self->index);
    }

    self->index++;

    return 1;
}

int bsal_vector_iterator_get_next_value(struct bsal_vector_iterator *self, void *value)
{
    void *bucket;
    int size;

    if (!bsal_vector_iterator_next(self, (void **)&bucket)) {
        return 0;
    }

    if (value != NULL) {
        size = bsal_vector_element_size(self->list);
        memcpy(value, bucket, size);
    }

    return 1;
}
