
#ifndef _BSAL_VECTOR_H
#define _BSAL_VECTOR_H

#include <stdint.h>

struct bsal_vector{
    void *data;
    int maximum_size;
    int size;
    int element_size;
};

void bsal_vector_init(struct bsal_vector *self, int element_size);
void bsal_vector_destroy(struct bsal_vector *self);
void bsal_vector_resize(struct bsal_vector *self, int size);
int bsal_vector_size(struct bsal_vector *self);
void *bsal_vector_at(struct bsal_vector *self, int index);
void bsal_vector_push_back(struct bsal_vector *self, void *data);
int bsal_vector_pack_size(struct bsal_vector *self);
void bsal_vector_pack(struct bsal_vector *self, void *buffer);
void bsal_vector_unpack(struct bsal_vector *self, void *buffer);
void bsal_vector_copy_range(struct bsal_vector *self, int first, int last, struct bsal_vector *other);
int bsal_vector_index_of(struct bsal_vector *self, void *data);

#endif
