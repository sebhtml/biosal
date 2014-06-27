
#ifndef BSAL_MAP_H
#define BSAL_MAP_H

#include "dynamic_hash_table.h"

struct bsal_map {
    struct bsal_dynamic_hash_table table;
};

void bsal_map_init(struct bsal_map *self, int key_size, int value_size);
void bsal_map_destroy(struct bsal_map *self);

void *bsal_map_add(struct bsal_map *self, void *key);
void *bsal_map_get(struct bsal_map *self, void *key);
int bsal_map_get_value(struct bsal_map *self, void *key, void *value);
void bsal_map_delete(struct bsal_map *self, void *key);
void bsal_map_add_value(struct bsal_map *self, void *key, void *value);

uint64_t bsal_map_size(struct bsal_map *self);
int bsal_map_get_key_size(struct bsal_map *self);
int bsal_map_get_value_size(struct bsal_map *self);

struct bsal_dynamic_hash_table *bsal_map_table(struct bsal_map *self);

int bsal_map_pack_size(struct bsal_map *self);
int bsal_map_pack(struct bsal_map *self, void *buffer);
int bsal_map_unpack(struct bsal_map *self, void *buffer);

int bsal_map_empty(struct bsal_map *self);

#endif
