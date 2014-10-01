
#include "hash_table_iterator.h"

#include <stdlib.h>

void biosal_hash_table_iterator_init(struct biosal_hash_table_iterator *self, struct biosal_hash_table *list)
{
    self->index = 0;
    self->list = list;
}

void biosal_hash_table_iterator_destroy(struct biosal_hash_table_iterator *self)
{
    self->index = 0;
    self->list = NULL;
}

int biosal_hash_table_iterator_has_next(struct biosal_hash_table_iterator *self)
{
    uint64_t size;

    if (self->list == NULL) {
        return 0;
    }

    size = biosal_hash_table_buckets(self->list);

    if (size == 0) {
        return 0;
    }

    while (self->index < size
                    && biosal_hash_table_state(self->list, self->index) !=
                    BIOSAL_HASH_TABLE_BUCKET_OCCUPIED) {
        self->index++;
    }

    if (self->index >= size) {
        return 0;
    }

    return 1;
}

void biosal_hash_table_iterator_next(struct biosal_hash_table_iterator *self, void **key, void **value)
{
    if (!biosal_hash_table_iterator_has_next(self)) {
        return;
    }

    if (key != NULL) {
        *key = biosal_hash_table_key(self->list, self->index);
    }

    if (value != NULL) {
        *value = biosal_hash_table_value(self->list, self->index);
    }

    self->index++;
}


