
#include "set_iterator.h"

#include "set.h"

#include <stdlib.h>
#include <string.h>

void biosal_set_iterator_init(struct biosal_set_iterator *self, struct biosal_set *list)
{
    self->list = biosal_set_map(list);
    biosal_map_iterator_init(&self->iterator, self->list);
}

void biosal_set_iterator_destroy(struct biosal_set_iterator *self)
{
    biosal_map_iterator_destroy(&self->iterator);
}

int biosal_set_iterator_has_next(struct biosal_set_iterator *self)
{
    return biosal_map_iterator_has_next(&self->iterator);
}

int biosal_set_iterator_next(struct biosal_set_iterator *self, void **key)
{
    if (!biosal_set_iterator_has_next(self)) {
        return 0;
    }

    biosal_map_iterator_next(&self->iterator, key, NULL);

    return 1;
}

int biosal_set_iterator_get_next_value(struct biosal_set_iterator *self, void *key)
{
    int size;
    void *bucket;

    if (!biosal_set_iterator_next(self, (void **)&bucket)) {
        return 0;
    }

    if (key != NULL) {
        size = biosal_map_get_key_size(self->list);
        biosal_memory_copy(key, bucket, size);
    }

    return 1;
}
