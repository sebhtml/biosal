
#ifndef BSAL_SET_ITERATOR_H
#define BSAL_SET_ITERATOR_H

#include "map_iterator.h"

struct bsal_set;

struct bsal_set_iterator {
    struct bsal_map_iterator iterator;
    struct bsal_map *list;
};

void bsal_set_iterator_init(struct bsal_set_iterator *self, struct bsal_set *list);
void bsal_set_iterator_destroy(struct bsal_set_iterator *self);

int bsal_set_iterator_has_next(struct bsal_set_iterator *self);
int bsal_set_iterator_next(struct bsal_set_iterator *self, void **key);
int bsal_set_iterator_get_next_value(struct bsal_set_iterator *self, void *key);

#endif
