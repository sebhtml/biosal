
#include "vector.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void bsal_vector_init(struct bsal_vector *self, int element_size)
{
    self->element_size = element_size;
    self->maximum_size = 0;
    self->size = 0;
    self->data = NULL;
}

void bsal_vector_destroy(struct bsal_vector *self)
{
    if (self->data != NULL) {
        free(self->data);
        self->data = NULL;
    }

    self->element_size = 0;
    self->maximum_size = 0;
    self->size = 0;
    self->data = NULL;
}

void bsal_vector_resize(struct bsal_vector *self, uint64_t size)
{
    void *new_data;

    if (size == self->size) {
        return;
    }

    /* implement shrinking */
    if (size < self->size) {
        self->size = size;
        return;
    }

    if (size <= self->maximum_size) {
        self->size = size;
        return;
    }

    if (size == 0) {
        return;
    }

    new_data = malloc(size * self->element_size);
    memcpy(new_data, self->data, self->size * self->element_size);
    free(self->data);
    self->data = new_data;
    self->size = size;
    self->maximum_size = size;
}

uint64_t bsal_vector_size(struct bsal_vector *self)
{
    return self->size;
}

void *bsal_vector_at(struct bsal_vector *self, uint64_t index)
{
    if (index >= self->size) {
        return NULL;
    }

    return (char *)self->data + index * self->element_size;
}

void bsal_vector_push_back(struct bsal_vector *self, void *data)
{
    uint64_t index;
    uint64_t new_maximum_size;
    void *bucket;
    void *new_data;

    index = self->size;

#ifdef BSAL_VECTOR_DEBUG8
    printf("index %d size %d max %d\n", (int)index,
                    (int)self->size, (int)self->maximum_size);
#endif

    if (self->size + 1 >= self->maximum_size) {

        new_maximum_size = 2 * self->maximum_size;

        if (new_maximum_size == 0) {
            new_maximum_size = 4;
        }

        new_data = malloc(new_maximum_size * self->element_size);
        if (self->size > 0) {
            memcpy(new_data, self->data, self->size * self->element_size);
            free(self->data);
        }

        self->data = new_data;
        self->maximum_size = new_maximum_size;

        bsal_vector_push_back(self, data);
        return;
    }

    self->size++;
    bucket = bsal_vector_at(self, index);
    memcpy(bucket, data, self->element_size);
}


