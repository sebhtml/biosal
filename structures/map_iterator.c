
#include "map_iterator.h"

#include "map.h"

#include <stdlib.h>

void bsal_map_iterator_init(struct bsal_map_iterator *self, struct bsal_map *list)
{
    bsal_dynamic_hash_table_iterator_init(&self->iterator, bsal_map_table(list));
}

void bsal_map_iterator_destroy(struct bsal_map_iterator *self)
{
    bsal_dynamic_hash_table_iterator_destroy(&self->iterator);
}

int bsal_map_iterator_has_next(struct bsal_map_iterator *self)
{
    return bsal_dynamic_hash_table_iterator_has_next(&self->iterator);
}

void bsal_map_iterator_next(struct bsal_map_iterator *self, void **key, void **value)
{
    bsal_dynamic_hash_table_iterator_next(&self->iterator, key, value);
}


