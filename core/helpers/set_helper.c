
#include "set_helper.h"

#include <core/structures/set_iterator.h>

int bsal_set_helper_get_any_int(struct bsal_set *self)
{
    int value;
    int *bucket;
    struct bsal_set_iterator iterator;

    value = -1;
    bsal_set_iterator_init(&iterator, self);

    if (bsal_set_iterator_has_next(&iterator)) {
        bsal_set_iterator_next(&iterator, (void **)&bucket);
        value = *bucket;
    }
    bsal_set_iterator_destroy(&iterator);

    return value;
}
