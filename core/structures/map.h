
#ifndef BIOSAL_MAP_H
#define BIOSAL_MAP_H

#include "dynamic_hash_table.h"

#include <core/system/memory.h>

struct biosal_memory_pool;

/*
 * This is map with keys and values.
 *
 * This is a hash table with built-in
 * alignment, auto-resize, and super
 * efficiency
 */
struct biosal_map {

#ifdef BIOSAL_MAP_ALIGNMENT_ENABLED
    int original_key_size;
    int original_value_size;
    void *key_buffer;
    int key_padding;
#endif

    struct biosal_dynamic_hash_table table;
};

void biosal_map_init(struct biosal_map *self, int key_size, int value_size);
void biosal_map_init_with_capacity(struct biosal_map *self, int key_size, int value_size, uint64_t buckets);

void biosal_map_destroy(struct biosal_map *self);

void *biosal_map_add(struct biosal_map *self, void *key);
void *biosal_map_get(struct biosal_map *self, void *key);
int biosal_map_get_value(struct biosal_map *self, void *key, void *value);
void biosal_map_delete(struct biosal_map *self, void *key);
int biosal_map_add_value(struct biosal_map *self, void *key, void *value);

/* Returns 1 if updated, 0 otherwise */
int biosal_map_update_value(struct biosal_map *self, void *key, void *value);

uint64_t biosal_map_size(struct biosal_map *self);
int biosal_map_get_key_size(struct biosal_map *self);
int biosal_map_get_value_size(struct biosal_map *self);

struct biosal_dynamic_hash_table *biosal_map_table(struct biosal_map *self);

int biosal_map_pack_size(struct biosal_map *self);
int biosal_map_pack(struct biosal_map *self, void *buffer);
int biosal_map_unpack(struct biosal_map *self, void *buffer);

int biosal_map_empty(struct biosal_map *self);
int biosal_map_pack_unpack(struct biosal_map *self, int operation, void *buffer);

#ifdef BIOSAL_MAP_ALIGNMENT_ENABLED
void *biosal_map_pad_key(struct biosal_map *self, void *key);
#endif

void biosal_map_set_memory_pool(struct biosal_map *self, struct biosal_memory_pool *memory);

void biosal_map_disable_deletion_support(struct biosal_map *self);
void biosal_map_enable_deletion_support(struct biosal_map *self);
void biosal_map_set_current_size_estimate(struct biosal_map *self, double value);
void biosal_map_set_threshold(struct biosal_map *self, double threshold);
int biosal_map_is_currently_resizing(struct biosal_map *self);

void biosal_map_clear(struct biosal_map *self);
void biosal_map_examine(struct biosal_map *self);

#endif
