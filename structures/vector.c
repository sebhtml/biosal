
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

    /* otherwise, the array needs to grow */
    bsal_vector_reserve(self, size);
    self->size = self->maximum_size;
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

    if (index < 0) {
        return NULL;
    }

    return ((char *)self->data) + index * self->element_size;
}

void bsal_vector_push_back(struct bsal_vector *self, void *data)
{
    int index;
    int new_maximum_size;
    void *bucket;

#ifdef BSAL_VECTOR_DEBUG
    printf("DEBUG bsal_vector_push_back size %d max %d\n",
                    (int)self->size, (int)self->maximum_size);
#endif

    if (self->size + 1 >= self->maximum_size) {

        new_maximum_size = 2 * self->maximum_size;

        if (new_maximum_size == 0) {
            new_maximum_size = 4;
        }

        bsal_vector_reserve(self, new_maximum_size);
        bsal_vector_push_back(self, data);
        return;
    }

    index = self->size;
    self->size++;
    bucket = bsal_vector_at(self, index);
    memcpy(bucket, data, self->element_size);

#ifdef BSAL_VECTOR_DEBUG
    printf("DEBUG bsal_vector_push_back new_size is %d, pushed in bucket %d, value in bucket %d\n",
                    self->size, index, *(int *)bucket);
#endif
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

    /* reserve space */
    bsal_vector_reserve(self, size);

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

void bsal_vector_copy_range(struct bsal_vector *self, int first, int last, struct bsal_vector *destination)
{
    int i;

    for (i = first; i <= last; i++) {

        bsal_vector_push_back(destination, bsal_vector_at(self, i));
    }
}

int bsal_vector_index_of(struct bsal_vector *self, void *data)
{
    int i;
    int last;

    last = bsal_vector_size(self) - 1;

    for (i = 0; i <= last; i++) {
        if (memcmp(bsal_vector_at(self, i), data, self->element_size) == 0) {
            return i;
        }
    }

    return -1;
}

int bsal_vector_at_as_int(struct bsal_vector *self, int index)
{
    int *bucket;

    bucket = (int *)bsal_vector_at(self, index);

    if (bucket == NULL) {
        return -1;
    }

    return *bucket;
}

char *bsal_vector_at_as_char_pointer(struct bsal_vector *self, int index)
{
    return (char *)bsal_vector_at_as_void_pointer(self, index);
}

void *bsal_vector_at_as_void_pointer(struct bsal_vector *self, int index)
{
    void **bucket;

    bucket = (void **)bsal_vector_at(self, index);

    if (bucket == NULL) {
        return NULL;
    }

    return *bucket;
}

void bsal_vector_reserve(struct bsal_vector *self, int size)
{
    void *new_data;
    int old_byte_count;
    int new_byte_count;

#ifdef BSAL_VECTOR_DEBUG
    printf("DEBUG bsal_vector_reserve %d buckets current_size %d\n", size,
                    self->size);
#endif

    if (size <= self->maximum_size) {
        return;
    }

    new_byte_count = size * self->element_size;
    old_byte_count = self->size * self->element_size;

#ifdef BSAL_VECTOR_DEBUG
    printf("DEBUG bsal_vector_reserve old_byte_count %d new_byte_count %d\n",
                    old_byte_count, new_byte_count);
#endif

    new_data = malloc(new_byte_count);

    /* copy old data */
    if (self->size > 0) {
        memcpy(new_data, self->data, old_byte_count);
        free(self->data);
    }

    self->data = new_data;
    self->maximum_size = size;
}

int bsal_vector_capacity(struct bsal_vector *self)
{
    return self->maximum_size;
}

void bsal_vector_update(struct bsal_vector *self, void *old_item, void *new_item)
{
    int i;
    int last;
    void *bucket;

    last = bsal_vector_size(self) - 1;

    for (i = 0; i <= last; i++) {
        bucket = bsal_vector_at(self, i);
        if (memcmp(bucket, old_item, self->element_size) == 0) {

            bsal_vector_set(self, i, new_item);
        }
    }
}

void bsal_vector_push_back_vector(struct bsal_vector *self, struct bsal_vector *other_vector)
{
    bsal_vector_copy_range(other_vector, 0, bsal_vector_size(other_vector) - 1, self);
}

void bsal_vector_set(struct bsal_vector *self, int index, void *data)
{
    void *bucket;

    bucket = bsal_vector_at(self, index);
    memcpy(bucket, data, self->element_size);
}

void bsal_vector_set_int(struct bsal_vector *self, int index, int value)
{
    bsal_vector_set(self, index, &value);
}

void bsal_vector_push_back_int(struct bsal_vector *self, int value)
{
    bsal_vector_push_back(self, &value);
}

void bsal_vector_print_int(struct bsal_vector *self)
{
    int i;
    int size;

    size = bsal_vector_size(self);
    i = 0;

    printf("[");
    while (i < size) {

        if (i > 0) {
            printf(", ");
        }
        printf("%d", bsal_vector_at_as_int(self, i));
        i++;
    }
    printf("]");
}
