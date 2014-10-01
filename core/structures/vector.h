
#ifndef CORE_VECTOR_H
#define CORE_VECTOR_H

#include <stdint.h>

/*
 * Include helper functions too
 */

#include "core/helpers/vector_helper.h"

struct core_memory_pool;

struct core_vector{
    void *data;
    int64_t maximum_size;
    int64_t size;
    int element_size;
    struct core_memory_pool *memory;

    int profile_allocate_calls;
    int profile_free_calls;
};

void core_vector_init(struct core_vector *self, int element_size);
void core_vector_destroy(struct core_vector *self);

void core_vector_init_copy(struct core_vector *self, struct core_vector *other);

void core_vector_resize(struct core_vector *self, int64_t size);
int64_t core_vector_size(struct core_vector *self);
void core_vector_reserve(struct core_vector *self, int64_t size);
int64_t core_vector_capacity(struct core_vector *self);

void *core_vector_at(struct core_vector *self, int64_t index);
int core_vector_get_value(struct core_vector *self, int64_t index, void *value);
void core_vector_set(struct core_vector *self, int64_t index, void *data);

void core_vector_push_back(struct core_vector *self, void *data);

int core_vector_pack_size(struct core_vector *self);
int core_vector_pack_unpack(struct core_vector *self, void *buffer, int operation);
int core_vector_pack(struct core_vector *self, void *buffer);
int core_vector_unpack(struct core_vector *self, void *buffer);

int64_t core_vector_index_of(struct core_vector *self, void *data);
void core_vector_update(struct core_vector *self, void *old_item, void *new_item);
int core_vector_element_size(struct core_vector *self);
void core_vector_set_memory_pool(struct core_vector *self, struct core_memory_pool *memory);

int core_vector_empty(struct core_vector *self);
void core_vector_clear(struct core_vector *self);

#endif
