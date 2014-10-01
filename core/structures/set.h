
#ifndef CORE_SET_H
#define CORE_SET_H

#include "map.h"

/*
 * This is a set with only keys.
 * Supported operations are:
 *
 * - add
 * - find
 * - delete
 */
struct core_set {
    struct core_map map;
};

void core_set_init(struct core_set *self, int key_size);
void core_set_destroy(struct core_set *self);

int core_set_add(struct core_set *self, void *key);
int core_set_find(struct core_set *self, void *key);
int core_set_delete(struct core_set *self, void *key);

uint64_t core_set_size(struct core_set *self);
int core_set_empty(struct core_set *self);

struct core_map *core_set_map(struct core_set *self);
void core_set_set_memory_pool(struct core_set *self, struct core_memory_pool *pool);

void core_set_clear(struct core_set *self);

#endif
