
#ifndef BIOSAL_VECTOR_ITERATOR_H
#define BIOSAL_VECTOR_ITERATOR_H

#include "vector.h"

#include <stdint.h>

struct biosal_vector_iterator {
    struct biosal_vector *list;
    int64_t index;
};

void biosal_vector_iterator_init(struct biosal_vector_iterator *self, struct biosal_vector *list);
void biosal_vector_iterator_destroy(struct biosal_vector_iterator *self);

int biosal_vector_iterator_has_next(struct biosal_vector_iterator *self);
int biosal_vector_iterator_next(struct biosal_vector_iterator *self, void **value);
int biosal_vector_iterator_get_next_value(struct biosal_vector_iterator *self, void *value);

#endif
