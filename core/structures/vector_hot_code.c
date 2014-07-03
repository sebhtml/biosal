
#include "vector.h"

#include <stdlib.h>
#include <string.h>

void *bsal_vector_at(struct bsal_vector *self, int64_t index)
{
    if (index >= self->size) {
        return NULL;
    }

    if (index < 0) {
        return NULL;
    }

    return ((char *)self->data) + index * self->element_size;
}

void bsal_vector_set(struct bsal_vector *self, int64_t index, void *data)
{
    void *bucket;

    bucket = bsal_vector_at(self, index);
    memcpy(bucket, data, self->element_size);
}

