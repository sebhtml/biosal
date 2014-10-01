
#ifndef CORE_VECTOR_HELPER_H
#define CORE_VECTOR_HELPER_H

#include <stdint.h>

struct core_vector;

void core_vector_print_int(struct core_vector *self);
void core_vector_set_int(struct core_vector *self, int64_t index, int value);
void core_vector_push_back_int(struct core_vector *self, int value);
void core_vector_push_back_uint64_t(struct core_vector *self, uint64_t value);
void core_vector_push_back_char(struct core_vector *self, char value);
int core_vector_at_as_int(struct core_vector *self, int64_t index);
float core_vector_at_as_float(struct core_vector *self, int64_t index);
uint64_t core_vector_at_as_uint64_t(struct core_vector *self, int64_t index);
char core_vector_at_as_char(struct core_vector *self, int64_t index);
char *core_vector_at_as_char_pointer(struct core_vector *self, int64_t index);
void *core_vector_at_as_void_pointer(struct core_vector *self, int64_t index);

void core_vector_sort_float(struct core_vector *self);
void core_vector_sort_float_reverse(struct core_vector *self);
int core_vector_compare_float(const void *a, const void *b);
int core_vector_compare_float_reverse(const void *a, const void *b);

void core_vector_sort_int(struct core_vector *self);
void core_vector_sort_int_reverse(struct core_vector *self);
int core_vector_compare_int(const void *a, const void *b);
int core_vector_compare_int_reverse(const void *a, const void *b);

typedef int (*core_compare_fn_t)(
    const void *value1,
    const void *value2
);

void core_vector_sort(struct core_vector *self,
                core_compare_fn_t compare);

void core_vector_quicksort(struct core_vector *self,
                int64_t first, int64_t last, core_compare_fn_t compare,
                void *saved_pivot_value);

int64_t core_vector_select_pivot(struct core_vector *self,
                int64_t first, int64_t last, core_compare_fn_t compare);

int64_t core_vector_partition(struct core_vector *self,
                int64_t first, int64_t last, core_compare_fn_t compare,
                void *saved_pivot_value);
void core_vector_swap(struct core_vector *self,
                int64_t index1, int64_t index2);
/* copy positions first to last from self to destination
 */
void core_vector_copy_range(struct core_vector *self, int64_t first, int64_t last, struct core_vector *destination);

/* append other_vector to self
 */
void core_vector_push_back_vector(struct core_vector *self, struct core_vector *other_vector);

void *core_vector_at_first(struct core_vector *self);
void *core_vector_at_middle(struct core_vector *self);
void *core_vector_at_last(struct core_vector *self);

#endif
