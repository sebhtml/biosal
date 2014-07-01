
#include "set_iterator.h"

#include "set.h"

#include <stdlib.h>
#include <string.h>

void bsal_set_iterator_init(struct bsal_set_iterator *self, struct bsal_set *list)
{
    self->list = bsal_set_map(list);
    bsal_map_iterator_init(&self->iterator, self->list);
}

void bsal_set_iterator_destroy(struct bsal_set_iterator *self)
{
    bsal_map_iterator_destroy(&self->iterator);
}

int bsal_set_iterator_has_next(struct bsal_set_iterator *self)
{
    return bsal_map_iterator_has_next(&self->iterator);
}

int bsal_set_iterator_next(struct bsal_set_iterator *self, void **key)
{
    if (!bsal_set_iterator_has_next(self)) {
        return 0;
    }

    bsal_map_iterator_next(&self->iterator, key, NULL);

    return 1;
}

int bsal_set_iterator_get_next_value(struct bsal_set_iterator *self, void *key)
{
    int size;
    void *bucket;

    if (!bsal_set_iterator_next(self, (void **)&bucket)) {
        return 0;
    }

    size = bsal_map_get_key_size(self->list);

    memcpy(key, bucket, size);

    return 1;
}
