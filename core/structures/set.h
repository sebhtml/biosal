
#ifndef BIOSAL_SET_H
#define BIOSAL_SET_H

#include "map.h"

/*
 * This is a set with only keys.
 * Supported operations are:
 *
 * - add
 * - find
 * - delete
 */
struct biosal_set {
    struct biosal_map map;
};

void biosal_set_init(struct biosal_set *self, int key_size);
void biosal_set_destroy(struct biosal_set *self);

int biosal_set_add(struct biosal_set *self, void *key);
int biosal_set_find(struct biosal_set *self, void *key);
int biosal_set_delete(struct biosal_set *self, void *key);

uint64_t biosal_set_size(struct biosal_set *self);
int biosal_set_empty(struct biosal_set *self);

struct biosal_map *biosal_set_map(struct biosal_set *self);
void biosal_set_set_memory_pool(struct biosal_set *self, struct biosal_memory_pool *pool);

void biosal_set_clear(struct biosal_set *self);

#endif
