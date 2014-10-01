
#include "vector_helper.h"

#include <core/structures/vector.h>

#include <core/system/memory.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MEMORY_VECTOR_HELPER 0xf16bc1f5

char biosal_vector_at_as_char(struct biosal_vector *self, int64_t index)
{
    char *bucket;

    bucket = NULL;
    bucket = (char *)biosal_vector_at(self, index);

    if (bucket == NULL) {
        return -1;
    }

    return *bucket;
}

/*
#define BIOSAL_VECTOR_HELPER_DEBUG
*/
int biosal_vector_at_as_int(struct biosal_vector *self, int64_t index)
{
    int *bucket;

    bucket = NULL;
    bucket = (int *)biosal_vector_at(self, index);

    if (bucket != NULL) {
        return *bucket;
    }

    return -1;
}

uint64_t biosal_vector_at_as_uint64_t(struct biosal_vector *self, int64_t index)
{
    uint64_t *bucket;

    bucket = NULL;
    bucket = (uint64_t *)biosal_vector_at(self, index);

    if (bucket == NULL) {
        return 0;
    }

    return *bucket;
}

char *biosal_vector_at_as_char_pointer(struct biosal_vector *self, int64_t index)
{
    return (char *)biosal_vector_at_as_void_pointer(self, index);
}

void *biosal_vector_at_as_void_pointer(struct biosal_vector *self, int64_t index)
{
    void **bucket;

    bucket = (void **)biosal_vector_at(self, index);

    if (bucket == NULL) {
        return NULL;
    }

    return *bucket;
}

void biosal_vector_print_int(struct biosal_vector *self)
{
    int64_t i;
    int64_t size;

    size = biosal_vector_size(self);
    i = 0;

    printf("[");
    while (i < size) {

        if (i > 0) {
            printf(", ");
        }
        printf("%d: %d", (int)i, biosal_vector_at_as_int(self, i));
        i++;
    }
    printf("]");
}

void biosal_vector_set_int(struct biosal_vector *self, int64_t index, int value)
{
    biosal_vector_set(self, index, &value);
}

void biosal_vector_push_back_char(struct biosal_vector *self, char value)
{
    biosal_vector_push_back(self, &value);
}

void biosal_vector_push_back_int(struct biosal_vector *self, int value)
{
    biosal_vector_push_back(self, &value);
}

void biosal_vector_push_back_uint64_t(struct biosal_vector *self, uint64_t value)
{
    biosal_vector_push_back(self, &value);
}

/** \see http://www.cplusplus.com/reference/cstdlib/qsort/
 */
int biosal_vector_compare_int(const void *a, const void *b)
{
    int a_value;
    int b_value;

    a_value = *(int *)a;
    b_value = *(int *)b;

    if (a_value < b_value) {
        return -1;
    } else if (a_value > b_value) {
        return 1;
    }

    return 0;
}

int biosal_vector_compare_int_reverse(const void *a, const void *b)
{
    return biosal_vector_compare_int(b, a);
}

void biosal_vector_sort_int_reverse(struct biosal_vector *self)
{
    biosal_vector_sort(self, biosal_vector_compare_int_reverse);
}

void biosal_vector_sort_int(struct biosal_vector *self)
{
    biosal_vector_sort(self, biosal_vector_compare_int);
}

void biosal_vector_sort(struct biosal_vector *self, biosal_compare_fn_t compare)
{
    void *saved_pivot_value;
    int element_size;

    element_size = biosal_vector_element_size(self);

    saved_pivot_value = biosal_memory_allocate(element_size, MEMORY_VECTOR_HELPER);

    biosal_vector_quicksort(self, 0, biosal_vector_size(self) - 1, compare, saved_pivot_value);

    biosal_memory_free(saved_pivot_value, MEMORY_VECTOR_HELPER);
}

void biosal_vector_quicksort(struct biosal_vector *self,
                int64_t first, int64_t last, biosal_compare_fn_t compare,
                void *saved_pivot_value)
{
    int64_t pivot_index;

    if (first >= last) {
        return;
    }

    pivot_index = biosal_vector_partition(self, first, last, compare, saved_pivot_value);

    biosal_vector_quicksort(self, first, pivot_index - 1, compare, saved_pivot_value);
    biosal_vector_quicksort(self, pivot_index + 1, last, compare, saved_pivot_value);

#ifdef BIOSAL_VECTOR_HELPER_DEBUG
    printf("after sorting first %d last %d ", first, last);
    biosal_vector_print_int(self);
    printf("\n");
#endif
}

/**
 * \see http://en.wikipedia.org/wiki/Quicksort
 */
int64_t biosal_vector_partition(struct biosal_vector *self,
                int64_t first, int64_t last, biosal_compare_fn_t compare,
                void *saved_pivot_value)
{
    int64_t pivot_index;
    void *pivot_value;
    int64_t store_index;
    void *other_value;
    int64_t i;
    int element_size;

    pivot_index = biosal_vector_select_pivot(self, first, last, compare);
    pivot_value = biosal_vector_at(self, pivot_index);
    element_size = biosal_vector_element_size(self);
    biosal_memory_copy(saved_pivot_value, pivot_value, element_size);

#ifdef BIOSAL_VECTOR_HELPER_DEBUG
    printf("DEBUG ENTER partition first %d last %d pivot_index %d pivot_value %d ", (int)first, (int)last, (int)pivot_index,
                    *(int *)pivot_value);
    biosal_vector_print_int(self);
    printf("\n");
#endif

#ifdef BIOSAL_VECTOR_HELPER_DEBUG
    printf("pivot_index %d pivot_value %d\n",
                    pivot_index, *(int *)pivot_value);
#endif

    biosal_vector_swap(self, pivot_index, last);

    store_index = first;

    for (i = first; i <= last - 1; i++) {

        other_value = biosal_vector_at(self, i);

        if (compare(other_value, saved_pivot_value) <= 0) {

            biosal_vector_swap(self, i, store_index);
            store_index = store_index + 1;
        }
    }

    biosal_vector_swap(self, store_index, last);

    pivot_index = store_index;

#ifdef BIOSAL_VECTOR_HELPER_DEBUG
    printf("DEBUG EXIT partition first %d last %d pivot_index %d pivot_value %d ", (int)first, (int)last, (int)pivot_index,
                    *(int *)pivot_value);
    biosal_vector_print_int(self);
    printf("\n");
#endif

    return pivot_index;
}

int64_t biosal_vector_select_pivot(struct biosal_vector *self,
                int64_t first, int64_t last, biosal_compare_fn_t compare)
{

    int64_t middle;

    void *first_value;
    void *last_value;
    void *middle_value;

    middle = first + (last - first) / 2;

    first_value = biosal_vector_at(self, first);
    last_value = biosal_vector_at(self, last);
    middle_value = biosal_vector_at(self, middle);

    if (compare(first_value, middle_value) <= 0 && compare(middle_value, last_value) <= 0) {
        return middle;

    } else if (compare(middle_value, first_value) <= 0 && compare(first_value, last_value) <= 0) {
        return first;
    }

    return last;
}

void biosal_vector_swap(struct biosal_vector *self,
                int64_t index1, int64_t index2)
{
    void *value1;
    void *value2;
    int i;
    char saved;
    int element_size;

    value1 = biosal_vector_at(self, index1);
    value2 = biosal_vector_at(self, index2);

    if (value1 == NULL || value2 == NULL) {
        return;
    }

    element_size = biosal_vector_element_size(self);

    /* swap 2 elements, byte by byte.
     */
    for (i = 0; i < element_size; i++) {
        saved = ((char *)value1)[i];
        ((char *)value1)[i] = ((char *)value2)[i];
        ((char *)value2)[i] = saved;
    }
}

float biosal_vector_at_as_float(struct biosal_vector *self, int64_t index)
{
    float *bucket;

    bucket = (float *)biosal_vector_at(self, index);

    if (bucket == NULL) {
        return -1;
    }

    return *bucket;
}

void biosal_vector_sort_float_reverse(struct biosal_vector *self)
{
    biosal_vector_sort(self, biosal_vector_compare_float_reverse);
}

void biosal_vector_sort_float(struct biosal_vector *self)
{
    biosal_vector_sort(self, biosal_vector_compare_float);
}

int biosal_vector_compare_float(const void *a, const void *b)
{
    float a_value;
    float b_value;

    a_value = *(float *)a;
    b_value = *(float *)b;

    if (a_value < b_value) {
        return -1;
    } else if (a_value > b_value) {
        return 1;
    }

    return 0;
}

int biosal_vector_compare_float_reverse(const void *a, const void *b)
{
    return biosal_vector_compare_float(b, a);
}

void biosal_vector_copy_range(struct biosal_vector *self, int64_t first, int64_t last, struct biosal_vector *destination)
{
    int64_t i;

    for (i = first; i <= last; i++) {

        biosal_vector_push_back(destination, biosal_vector_at(self, i));
    }
}

void biosal_vector_push_back_vector(struct biosal_vector *self, struct biosal_vector *other_vector)
{
    biosal_vector_copy_range(other_vector, 0, biosal_vector_size(other_vector) - 1, self);
}

void *biosal_vector_at_first(struct biosal_vector *self)
{
    return biosal_vector_at(self, 0);
}

void *biosal_vector_at_middle(struct biosal_vector *self)
{
    return biosal_vector_at(self, biosal_vector_size(self) - 1);
}

void *biosal_vector_at_last(struct biosal_vector *self)
{
    return biosal_vector_at(self, biosal_vector_size(self) / 2);
}
