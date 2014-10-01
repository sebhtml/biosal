
#include "hash_table_group_iterator.h"

#include <stdlib.h>

void core_hash_table_group_iterator_init(struct core_hash_table_group_iterator *self, struct core_hash_table_group *list,
                int size, int key_size, int value_size)
{
    self->list = list;
    self->index = 0;
    self->size = size;
    self->key_size = key_size;
    self->value_size = value_size;
}

void core_hash_table_group_iterator_destroy(struct core_hash_table_group_iterator *self)
{
    self->list = NULL;
    self->index = 0;
    self->size = 0;
}

int core_hash_table_group_iterator_has_next(struct core_hash_table_group_iterator *self)
{
    if (self->list == NULL || self->size == 0) {
        return 0;
    }

    while (self->index < self->size
                    && core_hash_table_group_state(self->list, self->index) !=
                    CORE_HASH_TABLE_BUCKET_OCCUPIED) {
        self->index++;
    }

    if (self->index < self->size) {
        return 1;
    }

    return 0;
}

void core_hash_table_group_iterator_next(struct core_hash_table_group_iterator *self, void **key, void **value)
{
    if (!core_hash_table_group_iterator_has_next(self)) {
        return;
    }

    if (key != NULL) {
        *key = core_hash_table_group_key(self->list, self->index, self->key_size, self->value_size);
    }

    if (value != NULL) {
        *value = core_hash_table_group_value(self->list, self->index, self->key_size, self->value_size);
    }

    self->index++;
}
