
#ifndef BSAL_MAP_ITERATOR_H
#define BSAL_MAP_ITERATOR_H

#include "dynamic_hash_table_iterator.h"

struct bsal_map;

struct bsal_map_iterator {
    struct bsal_dynamic_hash_table_iterator iterator;
    struct bsal_map *list;
};

void bsal_map_iterator_init(struct bsal_map_iterator *self, struct bsal_map *list);
void bsal_map_iterator_destroy(struct bsal_map_iterator *self);

int bsal_map_iterator_has_next(struct bsal_map_iterator *self);
int bsal_map_iterator_next(struct bsal_map_iterator *self, void **key, void **value);
int bsal_map_iterator_get_next_key_and_value(struct bsal_map_iterator *self, void *key, void *value);

#endif
