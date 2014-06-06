
#include "vector.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
#define BSAL_VECTOR_DEBUG
*/

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

void bsal_vector_resize(struct bsal_vector *self, int size)
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

int bsal_vector_size(struct bsal_vector *self)
{
    return self->size;
}

void *bsal_vector_at(struct bsal_vector *self, int index)
{
    if (index >= self->size) {
        return NULL;
    }

    return (char *)self->data + index * self->element_size;
}

void bsal_vector_push_back(struct bsal_vector *self, void *data)
{
    int index;
    int new_maximum_size;
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

int bsal_vector_pack_size(struct bsal_vector *self)
{
    return sizeof(self->size) + sizeof(self->element_size) + self->size * self->element_size;
}

void bsal_vector_pack(struct bsal_vector *self, void *buffer)
{
    int offset;
    int bytes;

    offset = 0;

    bytes = sizeof(self->size);
    memcpy((char *)buffer + offset, &self->size, bytes);
    offset += bytes;

    bytes = sizeof(self->element_size);
    memcpy((char *)buffer + offset, &self->element_size, bytes);
    offset += bytes;

    bytes = self->size * self->element_size;
    memcpy((char *)buffer + offset, self->data, bytes);
    offset += bytes;
}

void bsal_vector_unpack(struct bsal_vector *self, void *buffer)
{
    int offset;
    int bytes;
    int element_size;
    int size;
    int i;
    void *value;

    bsal_vector_destroy(self);

    offset = 0;

    bytes = sizeof(size);
    memcpy(&size, (char *)buffer + offset, bytes);
    offset += bytes;

    bytes = sizeof(element_size);
    memcpy(&element_size, (char *)buffer + offset, bytes);
    offset += bytes;

#ifdef BSAL_VECTOR_DEBUG
    printf("DEBUG bsal_vector_unpack size %d element_size %d\n", size, element_size);
#endif

    value = malloc(element_size);

    bsal_vector_init(self, element_size);

    for (i = 0; i < size; i++) {
        bytes = element_size;
        memcpy(value, (char *)buffer + offset, bytes);
        offset += bytes;

        bsal_vector_push_back(self, value);
    }

    free(value);
    value = NULL;

#ifdef BSAL_VECTOR_DEBUG
    printf("DEBUG bsal_vector_unpack unpack successful\n");
#endif
}

void bsal_vector_copy_range(struct bsal_vector *self, int first, int last, struct bsal_vector *other)
{
    int i;

    for (i = first; i <= last; i++) {

        bsal_vector_push_back(other, bsal_vector_at(self, i));
    }
}
