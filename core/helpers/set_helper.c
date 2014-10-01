
#include "set_helper.h"

#include <core/structures/set.h>

#include <core/structures/set_iterator.h>

int biosal_set_get_any_int(struct biosal_set *self)
{
    int value;
    int *bucket;
    struct biosal_set_iterator iterator;

    value = -1;
    biosal_set_iterator_init(&iterator, self);

    if (biosal_set_iterator_has_next(&iterator)) {
        biosal_set_iterator_next(&iterator, (void **)&bucket);
        value = *bucket;
    }
    biosal_set_iterator_destroy(&iterator);

    return value;
}

void biosal_set_add_int(struct biosal_set *self, int value)
{
    biosal_set_add(self, &value);
}
