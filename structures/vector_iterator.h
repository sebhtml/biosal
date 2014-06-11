
#ifndef BSAL_VECTOR_ITERATOR_H
#define BSAL_VECTOR_ITERATOR_H

#include "vector.h"

#include <stdint.h>

struct bsal_vector_iterator {
    struct bsal_vector *list;
    uint64_t index;
};

void bsal_vector_iterator_init(struct bsal_vector_iterator *self, struct bsal_vector *list);
void bsal_vector_iterator_destroy(struct bsal_vector_iterator *self);

int bsal_vector_iterator_has_next(struct bsal_vector_iterator *self);
void bsal_vector_iterator_next(struct bsal_vector_iterator *self, void **value);

#endif
