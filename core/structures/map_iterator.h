
#ifndef BIOSAL_MAP_ITERATOR_H
#define BIOSAL_MAP_ITERATOR_H

#include "dynamic_hash_table_iterator.h"

struct biosal_map;

struct biosal_map_iterator {
    struct biosal_dynamic_hash_table_iterator iterator;
    struct biosal_map *list;
};

void biosal_map_iterator_init(struct biosal_map_iterator *self, struct biosal_map *list);
void biosal_map_iterator_destroy(struct biosal_map_iterator *self);

int biosal_map_iterator_has_next(struct biosal_map_iterator *self);
int biosal_map_iterator_next(struct biosal_map_iterator *self, void **key, void **value);
int biosal_map_iterator_get_next_key_and_value(struct biosal_map_iterator *self, void *key, void *value);

#endif
