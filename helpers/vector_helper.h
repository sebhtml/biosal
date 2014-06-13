
#ifndef BSAL_VECTOR_HELPER_H
#define BSAL_VECTOR_HELPER_H

#include <stdint.h>

struct bsal_vector;

void bsal_vector_helper_print_int(struct bsal_vector *self);
void bsal_vector_helper_set_int(struct bsal_vector *self, int64_t index, int value);
void bsal_vector_helper_push_back_int(struct bsal_vector *self, int value);
int bsal_vector_helper_at_as_int(struct bsal_vector *self, int64_t index);
char *bsal_vector_helper_at_as_char_pointer(struct bsal_vector *self, int64_t index);
void *bsal_vector_helper_at_as_void_pointer(struct bsal_vector *self, int64_t index);

#endif
