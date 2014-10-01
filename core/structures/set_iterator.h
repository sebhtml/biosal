
#ifndef CORE_SET_ITERATOR_H
#define CORE_SET_ITERATOR_H

#include "map_iterator.h"

struct core_set;

struct core_set_iterator {
    struct core_map_iterator iterator;
    struct core_map *list;
};

void core_set_iterator_init(struct core_set_iterator *self, struct core_set *list);
void core_set_iterator_destroy(struct core_set_iterator *self);

int core_set_iterator_has_next(struct core_set_iterator *self);
int core_set_iterator_next(struct core_set_iterator *self, void **key);
int core_set_iterator_get_next_value(struct core_set_iterator *self, void *key);

#endif
