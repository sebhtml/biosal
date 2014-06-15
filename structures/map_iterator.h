
#ifndef BSAL_MAP_ITERATOR_H
#define BSAL_MAP_ITERATOR_H

#include "map.h"

struct bsal_map_iterator {
    struct bsal_map *list;
    uint64_t index;
};

void bsal_map_iterator_init(struct bsal_map_iterator *self, struct bsal_map *list);
void bsal_map_iterator_destroy(struct bsal_map_iterator *self);

int bsal_map_iterator_has_next(struct bsal_map_iterator *self);
void bsal_map_iterator_next(struct bsal_map_iterator *self, void **key, void **value);

#endif
