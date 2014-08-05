
#ifndef BSAL_VECTOR_HELPER_H
#define BSAL_VECTOR_HELPER_H

#include <stdint.h>

struct bsal_vector;

void bsal_vector_print_int(struct bsal_vector *self);
void bsal_vector_set_int(struct bsal_vector *self, int64_t index, int value);
void bsal_vector_push_back_int(struct bsal_vector *self, int value);
void bsal_vector_push_back_uint64_t(struct bsal_vector *self, uint64_t value);
void bsal_vector_push_back_char(struct bsal_vector *self, char value);
int bsal_vector_at_as_int(struct bsal_vector *self, int64_t index);
float bsal_vector_at_as_float(struct bsal_vector *self, int64_t index);
uint64_t bsal_vector_at_as_uint64_t(struct bsal_vector *self, int64_t index);
char bsal_vector_at_as_char(struct bsal_vector *self, int64_t index);
char *bsal_vector_at_as_char_pointer(struct bsal_vector *self, int64_t index);
void *bsal_vector_at_as_void_pointer(struct bsal_vector *self, int64_t index);

void bsal_vector_sort_float(struct bsal_vector *self);
void bsal_vector_sort_float_reverse(struct bsal_vector *self);
int bsal_vector_compare_float(const void *a, const void *b);
int bsal_vector_compare_float_reverse(const void *a, const void *b);

void bsal_vector_sort_int(struct bsal_vector *self);
void bsal_vector_sort_int_reverse(struct bsal_vector *self);
int bsal_vector_compare_int(const void *a, const void *b);
int bsal_vector_compare_int_reverse(const void *a, const void *b);

typedef int (*bsal_compare_fn_t)(
    const void *value1,
    const void *value2
);

void bsal_vector_sort(struct bsal_vector *self,
                bsal_compare_fn_t compare);

void bsal_vector_quicksort(struct bsal_vector *self,
                int64_t first, int64_t last, bsal_compare_fn_t compare,
                void *saved_pivot_value);

int64_t bsal_vector_select_pivot(struct bsal_vector *self,
                int64_t first, int64_t last, bsal_compare_fn_t compare);

int64_t bsal_vector_partition(struct bsal_vector *self,
                int64_t first, int64_t last, bsal_compare_fn_t compare,
                void *saved_pivot_value);
void bsal_vector_swap(struct bsal_vector *self,
                int64_t index1, int64_t index2);
/* copy positions first to last from self to destination
 */
void bsal_vector_copy_range(struct bsal_vector *self, int64_t first, int64_t last, struct bsal_vector *destination);

/* append other_vector to self
 */
void bsal_vector_push_back_vector(struct bsal_vector *self, struct bsal_vector *other_vector);



#endif
