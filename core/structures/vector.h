
#ifndef BIOSAL_VECTOR_H
#define BIOSAL_VECTOR_H

#include <stdint.h>

/*
 * Include helper functions too
 */

#include "core/helpers/vector_helper.h"

struct biosal_memory_pool;

struct biosal_vector{
    void *data;
    int64_t maximum_size;
    int64_t size;
    int element_size;
    struct biosal_memory_pool *memory;

    int profile_allocate_calls;
    int profile_free_calls;
};

void biosal_vector_init(struct biosal_vector *self, int element_size);
void biosal_vector_destroy(struct biosal_vector *self);

void biosal_vector_init_copy(struct biosal_vector *self, struct biosal_vector *other);

void biosal_vector_resize(struct biosal_vector *self, int64_t size);
int64_t biosal_vector_size(struct biosal_vector *self);
void biosal_vector_reserve(struct biosal_vector *self, int64_t size);
int64_t biosal_vector_capacity(struct biosal_vector *self);

void *biosal_vector_at(struct biosal_vector *self, int64_t index);
int biosal_vector_get_value(struct biosal_vector *self, int64_t index, void *value);
void biosal_vector_set(struct biosal_vector *self, int64_t index, void *data);

void biosal_vector_push_back(struct biosal_vector *self, void *data);

int biosal_vector_pack_size(struct biosal_vector *self);
int biosal_vector_pack_unpack(struct biosal_vector *self, void *buffer, int operation);
int biosal_vector_pack(struct biosal_vector *self, void *buffer);
int biosal_vector_unpack(struct biosal_vector *self, void *buffer);

int64_t biosal_vector_index_of(struct biosal_vector *self, void *data);
void biosal_vector_update(struct biosal_vector *self, void *old_item, void *new_item);
int biosal_vector_element_size(struct biosal_vector *self);
void biosal_vector_set_memory_pool(struct biosal_vector *self, struct biosal_memory_pool *memory);

int biosal_vector_empty(struct biosal_vector *self);
void biosal_vector_clear(struct biosal_vector *self);

#endif
