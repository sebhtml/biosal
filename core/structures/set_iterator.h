
#ifndef BIOSAL_SET_ITERATOR_H
#define BIOSAL_SET_ITERATOR_H

#include "map_iterator.h"

struct biosal_set;

struct biosal_set_iterator {
    struct biosal_map_iterator iterator;
    struct biosal_map *list;
};

void biosal_set_iterator_init(struct biosal_set_iterator *self, struct biosal_set *list);
void biosal_set_iterator_destroy(struct biosal_set_iterator *self);

int biosal_set_iterator_has_next(struct biosal_set_iterator *self);
int biosal_set_iterator_next(struct biosal_set_iterator *self, void **key);
int biosal_set_iterator_get_next_value(struct biosal_set_iterator *self, void *key);

#endif
