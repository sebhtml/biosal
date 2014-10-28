
#include "vector.h"

#include <core/system/packer.h>
#include <core/system/memory_pool.h>
#include <core/system/debugger.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <stdint.h>
#include <inttypes.h>

/*
#define CORE_VECTOR_DEBUG
*/

#define CORE_VECTOR_INITIAL_BUCKET_COUNT 1

void core_vector_init(struct core_vector *self, int element_size)
{
    self->element_size = element_size;
    self->maximum_size = 0;
    self->size = 0;
    self->data = NULL;

    core_vector_set_memory_pool(self, NULL);

    self->profile_allocate_calls = 0;
    self->profile_free_calls = 0;
}

void core_vector_destroy(struct core_vector *self)
{
    if (self->data != NULL) {
        core_memory_pool_free(self->memory, self->data);
        ++self->profile_free_calls;
        self->data = NULL;
    }

    CORE_DEBUGGER_ASSERT(self->profile_allocate_calls == self->profile_free_calls);

    self->element_size = 0;
    self->maximum_size = 0;
    self->size = 0;

    core_vector_set_memory_pool(self, NULL);
}

void core_vector_resize(struct core_vector *self, int64_t size)
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
    core_vector_reserve(self, size);
    self->size = self->maximum_size;

#ifdef CORE_VECTOR_DEBUG
    printf("DEBUG resized to %d\n", self->size);
#endif
}

int64_t core_vector_size(struct core_vector *self)
{
    CORE_DEBUGGER_ASSERT(self != NULL);

    return self->size;
}

void core_vector_push_back(struct core_vector *self, void *data)
{
    int64_t index;
    int64_t new_maximum_size;
    void *bucket;

    CORE_DEBUGGER_ASSERT(data != NULL);

#ifdef CORE_VECTOR_DEBUG
    printf("DEBUG core_vector_push_back size %d max %d\n",
                    (int)self->size, (int)self->maximum_size);
#endif

    if (self->size + 1 > self->maximum_size) {

        new_maximum_size = 2 * self->maximum_size;

        if (new_maximum_size == 0) {
            new_maximum_size = CORE_VECTOR_INITIAL_BUCKET_COUNT;
        }

        core_vector_reserve(self, new_maximum_size);
        core_vector_push_back(self, data);
        return;
    }

    index = self->size;
    self->size++;
    bucket = core_vector_at(self, index);
    core_memory_copy(bucket, data, self->element_size);

#ifdef CORE_VECTOR_DEBUG
    printf("DEBUG core_vector_push_back new_size is %d, pushed in bucket %d, value in bucket %d\n",
                    self->size, index, *(int *)bucket);
#endif
}

int core_vector_pack_size(struct core_vector *self)
{
    return core_vector_pack_unpack(self, NULL, CORE_PACKER_OPERATION_PACK_SIZE);
}

int core_vector_pack(struct core_vector *self, void *buffer)
{
    return core_vector_pack_unpack(self, buffer, CORE_PACKER_OPERATION_PACK);
}

int core_vector_unpack(struct core_vector *self, void *buffer)
{
    return core_vector_pack_unpack(self, buffer, CORE_PACKER_OPERATION_UNPACK);
}

int64_t core_vector_index_of(struct core_vector *self, void *data)
{
    int64_t i;
    int64_t last;

    last = core_vector_size(self) - 1;

    for (i = 0; i <= last; i++) {
        if (memcmp(core_vector_at(self, i), data, self->element_size) == 0) {
            return i;
        }
    }

    return -1;
}

void core_vector_reserve(struct core_vector *self, int64_t size)
{
    void *new_data;
    int64_t old_byte_count;
    int64_t new_byte_count;

#ifdef CORE_VECTOR_DEBUG
    printf("DEBUG core_vector_reserve %p %d buckets current_size %d\n",
                    (void *)self,
                    (int)size,
                    (int)self->size);
#endif

    if (size <= self->maximum_size) {
        return;
    }

    new_byte_count = size * self->element_size;
    old_byte_count = self->size * self->element_size;

#ifdef CORE_VECTOR_DEBUG
    printf("DEBUG core_vector_reserve old_byte_count %d new_byte_count %d\n",
                    old_byte_count, new_byte_count);
#endif

    new_data = core_memory_pool_allocate(self->memory, new_byte_count);
    ++self->profile_allocate_calls;

#ifdef CORE_VECTOR_DEBUG
    printf("DEBUG size %d old %p new %p\n", (int)self->size,
                    (void *)self->data, (void *)new_data);
#endif

    /*
     * copy old data
     */
    if (self->size > 0) {
        core_memory_copy(new_data, self->data, old_byte_count);
        core_memory_pool_free(self->memory, self->data);
        ++self->profile_free_calls;

        self->data = NULL;
    }

    self->data = new_data;
    self->maximum_size = size;
}

int64_t core_vector_capacity(struct core_vector *self)
{
    return self->maximum_size;
}

void core_vector_update(struct core_vector *self, void *old_item, void *new_item)
{
    int64_t i;
    int64_t last;
    void *bucket;

    last = core_vector_size(self) - 1;

    for (i = 0; i <= last; i++) {
        bucket = core_vector_at(self, i);
        if (memcmp(bucket, old_item, self->element_size) == 0) {

            core_vector_set(self, i, new_item);
        }
    }
}

int core_vector_pack_unpack(struct core_vector *self, void *buffer, int operation)
{
    struct core_packer packer;
    int64_t bytes;
    int size;
    struct core_memory_pool *memory;
    int byte_count;

    core_packer_init(&packer, operation, buffer);

    core_packer_process(&packer, &self->size, sizeof(self->size));

#ifdef CORE_VECTOR_DEBUG
    printf("DEBUG core_vector_pack_unpack operation %d size %d\n",
                    operation, self->size);
#endif

    core_packer_process(&packer, &self->element_size, sizeof(self->element_size));

#ifdef CORE_VECTOR_DEBUG
    printf("DEBUG core_vector_pack_unpack operation %d element_size %d\n",
                    operation, self->element_size);
#endif

    if (operation == CORE_PACKER_OPERATION_UNPACK) {

        size = self->size;
        memory = self->memory;
        core_vector_init(self, self->element_size);

        /*
         * Restore attributes.
         */
        self->size = size;
        self->maximum_size = self->size;
        self->memory = memory;

        if (self->size > 0) {
            self->data = core_memory_pool_allocate(self->memory, self->maximum_size * self->element_size);
            ++self->profile_allocate_calls;
        } else {
            self->data = NULL;
        }
    }

    if (self->size > 0) {

        CORE_DEBUGGER_ASSERT(self->element_size > 0);

        byte_count = self->size * self->element_size;

#ifdef CORE_DEBUGGER_ENABLE_ASSERT
        if (!(byte_count > 0)) {
            printf("size %" PRId64 " element_size %d\n",
                            self->size, self->element_size);
        }
#endif

        CORE_DEBUGGER_ASSERT(byte_count > 0);

        core_packer_process(&packer, self->data, byte_count);
    }

    bytes = core_packer_get_byte_count(&packer);
    core_packer_destroy(&packer);

    return bytes;
}

int core_vector_element_size(struct core_vector *self)
{
    return self->element_size;
}

void core_vector_init_copy(struct core_vector *self, struct core_vector *other)
{
    core_vector_init(self, core_vector_element_size(other));

    core_vector_push_back_vector(self, other);
}

int core_vector_get_value(struct core_vector *self, int64_t index, void *value)
{
    void *bucket;

    bucket = core_vector_at(self, index);

    if (bucket == NULL) {
        return 0;
    }

    core_memory_copy(value, bucket, self->element_size);

    return 1;
}

void core_vector_set_memory_pool(struct core_vector *self, struct core_memory_pool *memory)
{
    self->memory = memory;
}

void *core_vector_at(struct core_vector *self, int64_t index)
{
#if 0
    CORE_DEBUGGER_ASSERT(index < self->size);
#endif

    if (index >= self->size) {
        return NULL;
    }

    if (index < 0) {
        return NULL;
    }

    return ((char *)self->data) + index * self->element_size;
}

void core_vector_set(struct core_vector *self, int64_t index, void *data)
{
    void *bucket;

    bucket = core_vector_at(self, index);
    core_memory_copy(bucket, data, self->element_size);
}

int core_vector_empty(struct core_vector *self)
{
    return core_vector_size(self) == 0;
}

void core_vector_clear(struct core_vector *self)
{
    core_vector_resize(self, 0);
}
