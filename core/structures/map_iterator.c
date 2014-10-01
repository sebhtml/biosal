
#include "map_iterator.h"

#include "map.h"

#include <string.h>
#include <stdlib.h>

void core_map_iterator_init(struct core_map_iterator *self, struct core_map *list)
{
    self->list = list;
    core_dynamic_hash_table_iterator_init(&self->iterator, core_map_table(self->list));
}

void core_map_iterator_destroy(struct core_map_iterator *self)
{
    core_dynamic_hash_table_iterator_destroy(&self->iterator);
}

int core_map_iterator_has_next(struct core_map_iterator *self)
{
    return core_dynamic_hash_table_iterator_has_next(&self->iterator);
}

int core_map_iterator_next(struct core_map_iterator *self, void **key, void **value)
{
    if (!core_map_iterator_has_next(self)) {
        return 0;
    }

    core_dynamic_hash_table_iterator_next(&self->iterator, key, value);

    return 1;
}

int core_map_iterator_get_next_key_and_value(struct core_map_iterator *self, void *key, void *value)
{
    void *key_bucket;
    void *value_bucket;
    int key_size;
    int value_size;

    if (!core_map_iterator_next(self, (void **)&key_bucket, (void **)&value_bucket)) {
        return 0;
    }

    if (key != NULL) {
        key_size = core_map_get_key_size(self->list);
        core_memory_copy(key, key_bucket, key_size);
    }

    if (value != NULL) {
        value_size = core_map_get_value_size(self->list);
        core_memory_copy(value, value_bucket, value_size);
    }

    return 1;
}
