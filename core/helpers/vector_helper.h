
#ifndef BIOSAL_VECTOR_HELPER_H
#define BIOSAL_VECTOR_HELPER_H

#include <stdint.h>

struct biosal_vector;

void biosal_vector_print_int(struct biosal_vector *self);
void biosal_vector_set_int(struct biosal_vector *self, int64_t index, int value);
void biosal_vector_push_back_int(struct biosal_vector *self, int value);
void biosal_vector_push_back_uint64_t(struct biosal_vector *self, uint64_t value);
void biosal_vector_push_back_char(struct biosal_vector *self, char value);
int biosal_vector_at_as_int(struct biosal_vector *self, int64_t index);
float biosal_vector_at_as_float(struct biosal_vector *self, int64_t index);
uint64_t biosal_vector_at_as_uint64_t(struct biosal_vector *self, int64_t index);
char biosal_vector_at_as_char(struct biosal_vector *self, int64_t index);
char *biosal_vector_at_as_char_pointer(struct biosal_vector *self, int64_t index);
void *biosal_vector_at_as_void_pointer(struct biosal_vector *self, int64_t index);

void biosal_vector_sort_float(struct biosal_vector *self);
void biosal_vector_sort_float_reverse(struct biosal_vector *self);
int biosal_vector_compare_float(const void *a, const void *b);
int biosal_vector_compare_float_reverse(const void *a, const void *b);

void biosal_vector_sort_int(struct biosal_vector *self);
void biosal_vector_sort_int_reverse(struct biosal_vector *self);
int biosal_vector_compare_int(const void *a, const void *b);
int biosal_vector_compare_int_reverse(const void *a, const void *b);

typedef int (*biosal_compare_fn_t)(
    const void *value1,
    const void *value2
);

void biosal_vector_sort(struct biosal_vector *self,
                biosal_compare_fn_t compare);

void biosal_vector_quicksort(struct biosal_vector *self,
                int64_t first, int64_t last, biosal_compare_fn_t compare,
                void *saved_pivot_value);

int64_t biosal_vector_select_pivot(struct biosal_vector *self,
                int64_t first, int64_t last, biosal_compare_fn_t compare);

int64_t biosal_vector_partition(struct biosal_vector *self,
                int64_t first, int64_t last, biosal_compare_fn_t compare,
                void *saved_pivot_value);
void biosal_vector_swap(struct biosal_vector *self,
                int64_t index1, int64_t index2);
/* copy positions first to last from self to destination
 */
void biosal_vector_copy_range(struct biosal_vector *self, int64_t first, int64_t last, struct biosal_vector *destination);

/* append other_vector to self
 */
void biosal_vector_push_back_vector(struct biosal_vector *self, struct biosal_vector *other_vector);

void *biosal_vector_at_first(struct biosal_vector *self);
void *biosal_vector_at_middle(struct biosal_vector *self);
void *biosal_vector_at_last(struct biosal_vector *self);

#endif
