
#ifndef CORE_MAP_H
#define CORE_MAP_H

#include "dynamic_hash_table.h"

#include <core/system/memory.h>

struct core_memory_pool;

/*
 * This is map with keys and values.
 *
 * This is a hash table with built-in
 * alignment, auto-resize, and super
 * efficiency
 */
struct core_map {

#ifdef CORE_MAP_ALIGNMENT_ENABLED
    int original_key_size;
    int original_value_size;
    void *key_buffer;
    int key_padding;
#endif

    struct core_dynamic_hash_table table;
};

void core_map_init(struct core_map *self, int key_size, int value_size);
void core_map_init_with_capacity(struct core_map *self, int key_size, int value_size, uint64_t buckets);

void core_map_destroy(struct core_map *self);

void *core_map_add(struct core_map *self, void *key);
void *core_map_get(struct core_map *self, void *key);
int core_map_get_value(struct core_map *self, void *key, void *value);
void core_map_delete(struct core_map *self, void *key);
int core_map_add_value(struct core_map *self, void *key, void *value);

/* Returns 1 if updated, 0 otherwise */
int core_map_update_value(struct core_map *self, void *key, void *value);

uint64_t core_map_size(struct core_map *self);
int core_map_get_key_size(struct core_map *self);
int core_map_get_value_size(struct core_map *self);

struct core_dynamic_hash_table *core_map_table(struct core_map *self);

int core_map_pack_size(struct core_map *self);
int core_map_pack(struct core_map *self, void *buffer);
int core_map_unpack(struct core_map *self, void *buffer);

int core_map_empty(struct core_map *self);
int core_map_pack_unpack(struct core_map *self, int operation, void *buffer);

#ifdef CORE_MAP_ALIGNMENT_ENABLED
void *core_map_pad_key(struct core_map *self, void *key);
#endif

void core_map_set_memory_pool(struct core_map *self, struct core_memory_pool *memory);

void core_map_disable_deletion_support(struct core_map *self);
void core_map_enable_deletion_support(struct core_map *self);
void core_map_set_current_size_estimate(struct core_map *self, double value);
void core_map_set_threshold(struct core_map *self, double threshold);
int core_map_is_currently_resizing(struct core_map *self);

void core_map_clear(struct core_map *self);
void core_map_examine(struct core_map *self);

#endif
