
#include "vector.h"

#include <core/system/packer.h>
#include <core/system/memory.h>
#include <core/system/memory_pool.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
#define BSAL_VECTOR_DEBUG
*/

#define BSAL_VECTOR_INITIAL_BUCKET_COUNT 1

void bsal_vector_init(struct bsal_vector *self, int element_size)
{
    self->element_size = element_size;
    self->maximum_size = 0;
    self->size = 0;
    self->data = NULL;

    bsal_vector_set_memory_pool(self, NULL);
}

void bsal_vector_destroy(struct bsal_vector *self)
{
    if (self->data != NULL) {
        bsal_memory_pool_free(self->memory, self->data);
        self->data = NULL;
    }

    self->element_size = 0;
    self->maximum_size = 0;
    self->size = 0;

    bsal_vector_set_memory_pool(self, NULL);
}

void bsal_vector_resize(struct bsal_vector *self, int64_t size)
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

#ifdef BSAL_VECTOR_DEBUG
    printf("DEBUG resized to %d\n", self->size);
#endif
}

int64_t bsal_vector_size(struct bsal_vector *self)
{
    return self->size;
}

void bsal_vector_push_back(struct bsal_vector *self, void *data)
{
    int64_t index;
    int64_t new_maximum_size;
    void *bucket;

#ifdef BSAL_VECTOR_DEBUG
    printf("DEBUG bsal_vector_push_back size %d max %d\n",
                    (int)self->size, (int)self->maximum_size);
#endif

    if (self->size + 1 >= self->maximum_size) {

        new_maximum_size = 2 * self->maximum_size;

        if (new_maximum_size == 0) {
            new_maximum_size = BSAL_VECTOR_INITIAL_BUCKET_COUNT;
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
    return bsal_vector_pack_unpack(self, NULL, BSAL_PACKER_OPERATION_DRY_RUN);
}

int bsal_vector_pack(struct bsal_vector *self, void *buffer)
{
    return bsal_vector_pack_unpack(self, buffer, BSAL_PACKER_OPERATION_PACK);
}

int bsal_vector_unpack(struct bsal_vector *self, void *buffer)
{
    return bsal_vector_pack_unpack(self, buffer, BSAL_PACKER_OPERATION_UNPACK);
}

void bsal_vector_copy_range(struct bsal_vector *self, int64_t first, int64_t last, struct bsal_vector *destination)
{
    int64_t i;

    for (i = first; i <= last; i++) {

        bsal_vector_push_back(destination, bsal_vector_at(self, i));
    }
}

int64_t bsal_vector_index_of(struct bsal_vector *self, void *data)
{
    int64_t i;
    int64_t last;

    last = bsal_vector_size(self) - 1;

    for (i = 0; i <= last; i++) {
        if (memcmp(bsal_vector_at(self, i), data, self->element_size) == 0) {
            return i;
        }
    }

    return -1;
}

void bsal_vector_reserve(struct bsal_vector *self, int64_t size)
{
    void *new_data;
    int64_t old_byte_count;
    int64_t new_byte_count;

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

    new_data = bsal_memory_pool_allocate(self->memory, new_byte_count);

#ifdef BSAL_VECTOR_DEBUG
    printf("DEBUG size %d old %p new %p\n", (int)self->size,
                    (void *)self->data, (void *)new_data);
#endif

    /* copy old data */
    if (self->size > 0) {
        memcpy(new_data, self->data, old_byte_count);
        bsal_memory_pool_free(self->memory, self->data);
    }

    self->data = new_data;
    self->maximum_size = size;
}

int64_t bsal_vector_capacity(struct bsal_vector *self)
{
    return self->maximum_size;
}

void bsal_vector_update(struct bsal_vector *self, void *old_item, void *new_item)
{
    int64_t i;
    int64_t last;
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

int bsal_vector_pack_unpack(struct bsal_vector *self, void *buffer, int operation)
{
    struct bsal_packer packer;
    int64_t bytes;
    int size;
    struct bsal_memory_pool *memory;

    bsal_packer_init(&packer, operation, buffer);

    bsal_packer_work(&packer, &self->size, sizeof(self->size));

#ifdef BSAL_VECTOR_DEBUG
    printf("DEBUG bsal_vector_pack_unpack operation %d size %d\n",
                    operation, self->size);
#endif

    bsal_packer_work(&packer, &self->element_size, sizeof(self->element_size));

#ifdef BSAL_VECTOR_DEBUG
    printf("DEBUG bsal_vector_pack_unpack operation %d element_size %d\n",
                    operation, self->element_size);
#endif

    if (operation == BSAL_PACKER_OPERATION_UNPACK) {

        size = self->size;
        memory = self->memory;
        bsal_vector_init(self, self->element_size);

        self->size = size;
        self->maximum_size = self->size;
        self->memory = memory;

        if (self->size > 0) {
            self->data = bsal_memory_pool_allocate(self->memory, self->maximum_size * self->element_size);
        } else {
            self->data = NULL;
        }
    }

    if (self->size > 0) {

        bsal_packer_work(&packer, self->data, self->size * self->element_size);
    }

    bytes = bsal_packer_worked_bytes(&packer);
    bsal_packer_destroy(&packer);

    return bytes;
}

int bsal_vector_element_size(struct bsal_vector *self)
{
    return self->element_size;
}

void bsal_vector_init_copy(struct bsal_vector *self, struct bsal_vector *other)
{
    bsal_vector_init(self, bsal_vector_element_size(other));

    bsal_vector_push_back_vector(self, other);
}

int bsal_vector_get_value(struct bsal_vector *self, int64_t index, void *value)
{
    void *bucket;

    bucket = bsal_vector_at(self, index);

    if (bucket == NULL) {
        return 0;
    }

    memcpy(value, bucket, self->element_size);

    return 1;
}

void bsal_vector_set_memory_pool(struct bsal_vector *vector, struct bsal_memory_pool *memory)
{
    vector->memory = NULL;
}

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

