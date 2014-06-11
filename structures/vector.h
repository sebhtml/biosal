
#ifndef BSAL_VECTOR_H
#define BSAL_VECTOR_H

#include <stdint.h>

struct bsal_vector{
    void *data;
    int64_t maximum_size;
    int64_t size;
    int element_size;
};

void bsal_vector_init(struct bsal_vector *self, int element_size);
void bsal_vector_destroy(struct bsal_vector *self);

void bsal_vector_resize(struct bsal_vector *self, int64_t size);
int64_t bsal_vector_size(struct bsal_vector *self);
void bsal_vector_reserve(struct bsal_vector *self, int64_t size);
int64_t bsal_vector_capacity(struct bsal_vector *self);

void *bsal_vector_at(struct bsal_vector *self, int64_t index);
void bsal_vector_set(struct bsal_vector *self, int64_t index, void *data);
void bsal_vector_set_int(struct bsal_vector *self, int64_t index, int value);

int bsal_vector_at_as_int(struct bsal_vector *self, int64_t index);
char *bsal_vector_at_as_char_pointer(struct bsal_vector *self, int64_t index);
void *bsal_vector_at_as_void_pointer(struct bsal_vector *self, int64_t index);

void bsal_vector_push_back(struct bsal_vector *self, void *data);
void bsal_vector_push_back_int(struct bsal_vector *self, int value);
/* append other_vector to self
 */
void bsal_vector_push_back_vector(struct bsal_vector *self, struct bsal_vector *other_vector);

int bsal_vector_pack_size(struct bsal_vector *self);
int bsal_vector_pack_unpack(struct bsal_vector *self, void *buffer, int operation);
int bsal_vector_pack(struct bsal_vector *self, void *buffer);
int bsal_vector_unpack(struct bsal_vector *self, void *buffer);

/* copy positions first to last from self to destination
 */
void bsal_vector_copy_range(struct bsal_vector *self, int64_t first, int64_t last, struct bsal_vector *destination);

int64_t bsal_vector_index_of(struct bsal_vector *self, void *data);
void bsal_vector_update(struct bsal_vector *self, void *old_item, void *new_item);
void bsal_vector_print_int(struct bsal_vector *self);

#endif
