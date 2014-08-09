
#ifndef BSAL_SET_H
#define BSAL_SET_H

#include "map.h"

/*
 * This is a set with only keys.
 * Supported operations are:
 *
 * - add
 * - find
 * - delete
 */
struct bsal_set {
    struct bsal_map map;
};

void bsal_set_init(struct bsal_set *self, int key_size);
void bsal_set_destroy(struct bsal_set *self);

int bsal_set_add(struct bsal_set *self, void *key);
int bsal_set_find(struct bsal_set *self, void *key);
int bsal_set_delete(struct bsal_set *self, void *key);

uint64_t bsal_set_size(struct bsal_set *self);
int bsal_set_empty(struct bsal_set *self);

struct bsal_map *bsal_set_map(struct bsal_set *self);
void bsal_set_set_memory_pool(struct bsal_set *self, struct bsal_memory_pool *pool);

void bsal_set_clear(struct bsal_set *self);

#endif
