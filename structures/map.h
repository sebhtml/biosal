
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
void bsal_map_delete(struct bsal_map *self, void *key);

uint64_t bsal_map_size(struct bsal_map *self);
uint64_t bsal_map_buckets(struct bsal_map *self);

int bsal_map_state(struct bsal_map *self, uint64_t bucket);
void *bsal_map_key(struct bsal_map *self, uint64_t bucket);
void *bsal_map_value(struct bsal_map *self, uint64_t bucket);

#endif
