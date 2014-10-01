
#include "set_helper.h"

#include <core/structures/set.h>

#include <core/structures/set_iterator.h>

int core_set_get_any_int(struct core_set *self)
{
    int value;
    int *bucket;
    struct core_set_iterator iterator;

    value = -1;
    core_set_iterator_init(&iterator, self);

    if (core_set_iterator_has_next(&iterator)) {
        core_set_iterator_next(&iterator, (void **)&bucket);
        value = *bucket;
    }
    core_set_iterator_destroy(&iterator);

    return value;
}

void core_set_add_int(struct core_set *self, int value)
{
    core_set_add(self, &value);
}
