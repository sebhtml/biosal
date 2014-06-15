
#include "map_iterator.h"

#include <stdlib.h>

void bsal_map_iterator_init(struct bsal_map_iterator *self, struct bsal_map *list)
{
    self->index = 0;
    self->list = list;
}

void bsal_map_iterator_destroy(struct bsal_map_iterator *self)
{
    self->index = 0;
    self->list = NULL;
}

int bsal_map_iterator_has_next(struct bsal_map_iterator *self)
{
    uint64_t size;

    if (self->list == NULL) {
        return 0;
    }

    size = bsal_map_buckets(self->list);

    if (size == 0) {
        return 0;
    }

    while (self->index < size
                    && bsal_map_state(self->list, self->index) !=
                    BSAL_HASH_TABLE_BUCKET_OCCUPIED) {
        self->index++;
    }

    if (self->index >= size) {
        return 0;
    }

    return 1;
}

void bsal_map_iterator_next(struct bsal_map_iterator *self, void **key, void **value)
{
    if (!bsal_map_iterator_has_next(self)) {
        return;
    }

    if (key != NULL) {
        *key = bsal_map_key(self->list, self->index);
    }

    if (value != NULL) {
        *value = bsal_map_value(self->list, self->index);
    }

    self->index++;
}


