
#include "hash_table_iterator.h"

#include <stdlib.h>

void core_hash_table_iterator_init(struct core_hash_table_iterator *self, struct core_hash_table *list)
{
    self->index = 0;
    self->list = list;
}

void core_hash_table_iterator_destroy(struct core_hash_table_iterator *self)
{
    self->index = 0;
    self->list = NULL;
}

int core_hash_table_iterator_has_next(struct core_hash_table_iterator *self)
{
    uint64_t size;

    if (self->list == NULL) {
        return 0;
    }

    size = core_hash_table_buckets(self->list);

    if (size == 0) {
        return 0;
    }

    while (self->index < size
                    && core_hash_table_state(self->list, self->index) !=
                    CORE_HASH_TABLE_BUCKET_OCCUPIED) {
        self->index++;
    }

    if (self->index >= size) {
        return 0;
    }

    return 1;
}

void core_hash_table_iterator_next(struct core_hash_table_iterator *self, void **key, void **value)
{
    if (!core_hash_table_iterator_has_next(self)) {
        return;
    }

    if (key != NULL) {
        *key = core_hash_table_key(self->list, self->index);
    }

    if (value != NULL) {
        *value = core_hash_table_value(self->list, self->index);
    }

    self->index++;
}


