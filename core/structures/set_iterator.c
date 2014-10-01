
#include "set_iterator.h"

#include "set.h"

#include <stdlib.h>
#include <string.h>

void core_set_iterator_init(struct core_set_iterator *self, struct core_set *list)
{
    self->list = core_set_map(list);
    core_map_iterator_init(&self->iterator, self->list);
}

void core_set_iterator_destroy(struct core_set_iterator *self)
{
    core_map_iterator_destroy(&self->iterator);
}

int core_set_iterator_has_next(struct core_set_iterator *self)
{
    return core_map_iterator_has_next(&self->iterator);
}

int core_set_iterator_next(struct core_set_iterator *self, void **key)
{
    if (!core_set_iterator_has_next(self)) {
        return 0;
    }

    core_map_iterator_next(&self->iterator, key, NULL);

    return 1;
}

int core_set_iterator_get_next_value(struct core_set_iterator *self, void *key)
{
    int size;
    void *bucket;

    if (!core_set_iterator_next(self, (void **)&bucket)) {
        return 0;
    }

    if (key != NULL) {
        size = core_map_get_key_size(self->list);
        core_memory_copy(key, bucket, size);
    }

    return 1;
}
