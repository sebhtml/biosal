
#ifndef CORE_MAP_ITERATOR_H
#define CORE_MAP_ITERATOR_H

#include "dynamic_hash_table_iterator.h"

struct core_map;

struct core_map_iterator {
    struct core_dynamic_hash_table_iterator iterator;
    struct core_map *list;
};

void core_map_iterator_init(struct core_map_iterator *self, struct core_map *list);
void core_map_iterator_destroy(struct core_map_iterator *self);

int core_map_iterator_has_next(struct core_map_iterator *self);
int core_map_iterator_next(struct core_map_iterator *self, void **key, void **value);
int core_map_iterator_get_next_key_and_value(struct core_map_iterator *self, void *key, void *value);

#endif
