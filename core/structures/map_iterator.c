
#include "map_iterator.h"

#include "map.h"

#include <string.h>
#include <stdlib.h>

void biosal_map_iterator_init(struct biosal_map_iterator *self, struct biosal_map *list)
{
    self->list = list;
    biosal_dynamic_hash_table_iterator_init(&self->iterator, biosal_map_table(self->list));
}

void biosal_map_iterator_destroy(struct biosal_map_iterator *self)
{
    biosal_dynamic_hash_table_iterator_destroy(&self->iterator);
}

int biosal_map_iterator_has_next(struct biosal_map_iterator *self)
{
    return biosal_dynamic_hash_table_iterator_has_next(&self->iterator);
}

int biosal_map_iterator_next(struct biosal_map_iterator *self, void **key, void **value)
{
    if (!biosal_map_iterator_has_next(self)) {
        return 0;
    }

    biosal_dynamic_hash_table_iterator_next(&self->iterator, key, value);

    return 1;
}

int biosal_map_iterator_get_next_key_and_value(struct biosal_map_iterator *self, void *key, void *value)
{
    void *key_bucket;
    void *value_bucket;
    int key_size;
    int value_size;

    if (!biosal_map_iterator_next(self, (void **)&key_bucket, (void **)&value_bucket)) {
        return 0;
    }

    if (key != NULL) {
        key_size = biosal_map_get_key_size(self->list);
        biosal_memory_copy(key, key_bucket, key_size);
    }

    if (value != NULL) {
        value_size = biosal_map_get_value_size(self->list);
        biosal_memory_copy(value, value_bucket, value_size);
    }

    return 1;
}
