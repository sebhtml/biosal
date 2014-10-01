
#ifndef CORE_VECTOR_ITERATOR_H
#define CORE_VECTOR_ITERATOR_H

#include "vector.h"

#include <stdint.h>

struct core_vector_iterator {
    struct core_vector *list;
    int64_t index;
};

void core_vector_iterator_init(struct core_vector_iterator *self, struct core_vector *list);
void core_vector_iterator_destroy(struct core_vector_iterator *self);

int core_vector_iterator_has_next(struct core_vector_iterator *self);
int core_vector_iterator_next(struct core_vector_iterator *self, void **value);
int core_vector_iterator_get_next_value(struct core_vector_iterator *self, void *value);

#endif
