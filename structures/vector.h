
#ifndef _BSAL_VECTOR_H
#define _BSAL_VECTOR_H

#include <stdint.h>

struct bsal_vector{
    void *data;
    uint64_t maximum_size;
    uint64_t size;
    int element_size;
};

void bsal_vector_init(struct bsal_vector *self, int element_size);
void bsal_vector_destroy(struct bsal_vector *self);
void bsal_vector_resize(struct bsal_vector *self, uint64_t size);
uint64_t bsal_vector_size(struct bsal_vector *self);
void *bsal_vector_at(struct bsal_vector *self, uint64_t index);
void bsal_vector_push_back(struct bsal_vector *self, void *data);

#endif
