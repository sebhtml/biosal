
#ifndef BSAL_MAP_H
#define BSAL_MAP_H

#include "dynamic_hash_table.h"

#include <core/system/memory.h>

struct bsal_memory_pool;

/*
 * This is a hash table with built-in
 * alignment, auto-resize, and super
 * efficiency
 */
struct bsal_map {

#ifdef BSAL_MEMORY_ALIGNMENT_ENABLED
    int original_key_size;
    int original_value_size;
    void *key_buffer;
    int key_padding;
#endif

    struct bsal_dynamic_hash_table table;
};

void bsal_map_init(struct bsal_map *self, int key_size, int value_size);
void bsal_map_destroy(struct bsal_map *self);

void *bsal_map_add(struct bsal_map *self, void *key);
void *bsal_map_get(struct bsal_map *self, void *key);
int bsal_map_get_value(struct bsal_map *self, void *key, void *value);
void bsal_map_delete(struct bsal_map *self, void *key);
int bsal_map_add_value(struct bsal_map *self, void *key, void *value);
int bsal_map_update_value(struct bsal_map *self, void *key, void *value);

uint64_t bsal_map_size(struct bsal_map *self);
int bsal_map_get_key_size(struct bsal_map *self);
int bsal_map_get_value_size(struct bsal_map *self);

struct bsal_dynamic_hash_table *bsal_map_table(struct bsal_map *self);

int bsal_map_pack_size(struct bsal_map *self);
int bsal_map_pack(struct bsal_map *self, void *buffer);
int bsal_map_unpack(struct bsal_map *self, void *buffer);

int bsal_map_empty(struct bsal_map *self);
int bsal_map_pack_unpack(struct bsal_map *self, int operation, void *buffer);

#ifdef BSAL_MEMORY_ALIGNMENT_ENABLED
void *bsal_map_pad_key(struct bsal_map *self, void *key);
#endif

void bsal_map_set_memory_pool(struct bsal_map *map, struct bsal_memory_pool *memory);

#endif
