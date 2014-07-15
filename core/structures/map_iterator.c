
#include "map_iterator.h"

#include "map.h"

#include <string.h>
#include <stdlib.h>

void bsal_map_iterator_init(struct bsal_map_iterator *self, struct bsal_map *list)
{
    self->list = list;
    bsal_dynamic_hash_table_iterator_init(&self->iterator, bsal_map_table(self->list));
}

void bsal_map_iterator_destroy(struct bsal_map_iterator *self)
{
    bsal_dynamic_hash_table_iterator_destroy(&self->iterator);
}

int bsal_map_iterator_has_next(struct bsal_map_iterator *self)
{
    return bsal_dynamic_hash_table_iterator_has_next(&self->iterator);
}

int bsal_map_iterator_next(struct bsal_map_iterator *self, void **key, void **value)
{
    if (!bsal_map_iterator_has_next(self)) {
        return 0;
    }

    bsal_dynamic_hash_table_iterator_next(&self->iterator, key, value);

    return 1;
}

int bsal_map_iterator_get_next_key_and_value(struct bsal_map_iterator *self, void *key, void *value)
{
    void *key_bucket;
    void *value_bucket;
    int key_size;
    int value_size;

    if (!bsal_map_iterator_next(self, (void **)&key_bucket, (void **)&value_bucket)) {
        return 0;
    }

    if (key != NULL) {
        key_size = bsal_map_get_key_size(self->list);
        memcpy(key, key_bucket, key_size);
    }

    if (value != NULL) {
        value_size = bsal_map_get_value_size(self->list);
        memcpy(value, value_bucket, value_size);
    }

    return 1;
}
