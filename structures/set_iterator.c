
#include "set_iterator.h"

#include "set.h"

#include <stdlib.h>

void bsal_set_iterator_init(struct bsal_set_iterator *self, struct bsal_set *list)
{
    bsal_map_iterator_init(&self->iterator, bsal_set_map(list));
}

void bsal_set_iterator_destroy(struct bsal_set_iterator *self)
{
    bsal_map_iterator_destroy(&self->iterator);
}

int bsal_set_iterator_has_next(struct bsal_set_iterator *self)
{
    return bsal_map_iterator_has_next(&self->iterator);
}

void bsal_set_iterator_next(struct bsal_set_iterator *self, void **key)
{
    bsal_map_iterator_next(&self->iterator, key, NULL);
}

