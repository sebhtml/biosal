
#include "vector_helper.h"

#include <core/structures/vector.h>

#include <core/system/memory.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

char bsal_vector_helper_at_as_char(struct bsal_vector *self, int64_t index)
{
    char *bucket;

    bucket = NULL;
    bucket = (char *)bsal_vector_at(self, index);

    if (bucket == NULL) {
        return -1;
    }

    return *bucket;
}

/*
#define BSAL_VECTOR_HELPER_DEBUG
*/
int bsal_vector_helper_at_as_int(struct bsal_vector *self, int64_t index)
{
    int *bucket;

    bucket = NULL;
    bucket = (int *)bsal_vector_at(self, index);

    if (bucket != NULL) {
        return *bucket;
    }

    return -1;
}

uint64_t bsal_vector_helper_at_as_uint64_t(struct bsal_vector *self, int64_t index)
{
    uint64_t *bucket;

    bucket = NULL;
    bucket = (uint64_t *)bsal_vector_at(self, index);

    if (bucket == NULL) {
        return 0;
    }

    return *bucket;
}

char *bsal_vector_helper_at_as_char_pointer(struct bsal_vector *self, int64_t index)
{
    return (char *)bsal_vector_helper_at_as_void_pointer(self, index);
}

void *bsal_vector_helper_at_as_void_pointer(struct bsal_vector *self, int64_t index)
{
    void **bucket;

    bucket = (void **)bsal_vector_at(self, index);

    if (bucket == NULL) {
        return NULL;
    }

    return *bucket;
}

void bsal_vector_helper_print_int(struct bsal_vector *self)
{
    int64_t i;
    int64_t size;

    size = bsal_vector_size(self);
    i = 0;

    printf("[");
    while (i < size) {

        if (i > 0) {
            printf(", ");
        }
        printf("%d: %d", (int)i, bsal_vector_helper_at_as_int(self, i));
        i++;
    }
    printf("]");
}

void bsal_vector_helper_set_int(struct bsal_vector *self, int64_t index, int value)
{
    bsal_vector_set(self, index, &value);
}

void bsal_vector_helper_push_back_char(struct bsal_vector *self, char value)
{
    bsal_vector_push_back(self, &value);
}

void bsal_vector_helper_push_back_int(struct bsal_vector *self, int value)
{
    bsal_vector_push_back(self, &value);
}

void bsal_vector_helper_push_back_uint64_t(struct bsal_vector *self, uint64_t value)
{
    bsal_vector_push_back(self, &value);
}

/** \see http://www.cplusplus.com/reference/cstdlib/qsort/
 */
int bsal_vector_helper_compare_int(const void *a, const void *b)
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

int bsal_vector_helper_compare_int_reverse(const void *a, const void *b)
{
    return bsal_vector_helper_compare_int(b, a);
}

void bsal_vector_helper_sort_int_reverse(struct bsal_vector *self)
{
    bsal_vector_helper_sort(self, bsal_vector_helper_compare_int_reverse);
}

void bsal_vector_helper_sort_int(struct bsal_vector *self)
{
    bsal_vector_helper_sort(self, bsal_vector_helper_compare_int);
}

void bsal_vector_helper_sort(struct bsal_vector *self, bsal_compare_fn_t compare)
{
    void *saved_pivot_value;
    int element_size;

    element_size = bsal_vector_element_size(self);

    saved_pivot_value = bsal_memory_allocate(element_size);

    bsal_vector_helper_quicksort(self, 0, bsal_vector_size(self) - 1, compare, saved_pivot_value);

    bsal_memory_free(saved_pivot_value);
}

void bsal_vector_helper_quicksort(struct bsal_vector *self,
                int64_t first, int64_t last, bsal_compare_fn_t compare,
                void *saved_pivot_value)
{
    int64_t pivot_index;

    if (first >= last) {
        return;
    }

    pivot_index = bsal_vector_helper_partition(self, first, last, compare, saved_pivot_value);

    bsal_vector_helper_quicksort(self, first, pivot_index - 1, compare, saved_pivot_value);
    bsal_vector_helper_quicksort(self, pivot_index + 1, last, compare, saved_pivot_value);

#ifdef BSAL_VECTOR_HELPER_DEBUG
    printf("after sorting first %d last %d ", first, last);
    bsal_vector_helper_print_int(self);
    printf("\n");
#endif
}

/**
 * \see http://en.wikipedia.org/wiki/Quicksort
 */
int64_t bsal_vector_helper_partition(struct bsal_vector *self,
                int64_t first, int64_t last, bsal_compare_fn_t compare,
                void *saved_pivot_value)
{
    int64_t pivot_index;
    void *pivot_value;
    int64_t store_index;
    void *other_value;
    int64_t i;
    int element_size;

    pivot_index = bsal_vector_helper_select_pivot(self, first, last, compare);
    pivot_value = bsal_vector_at(self, pivot_index);
    element_size = bsal_vector_element_size(self);
    memcpy(saved_pivot_value, pivot_value, element_size);

#ifdef BSAL_VECTOR_HELPER_DEBUG
    printf("DEBUG ENTER partition first %d last %d pivot_index %d pivot_value %d ", (int)first, (int)last, (int)pivot_index,
                    *(int *)pivot_value);
    bsal_vector_helper_print_int(self);
    printf("\n");
#endif

#ifdef BSAL_VECTOR_HELPER_DEBUG
    printf("pivot_index %d pivot_value %d\n",
                    pivot_index, *(int *)pivot_value);
#endif

    bsal_vector_helper_swap(self, pivot_index, last);

    store_index = first;

    for (i = first; i <= last - 1; i++) {

        other_value = bsal_vector_at(self, i);

        if (compare(other_value, saved_pivot_value) <= 0) {

            bsal_vector_helper_swap(self, i, store_index);
            store_index = store_index + 1;
        }
    }

    bsal_vector_helper_swap(self, store_index, last);

    pivot_index = store_index;

#ifdef BSAL_VECTOR_HELPER_DEBUG
    printf("DEBUG EXIT partition first %d last %d pivot_index %d pivot_value %d ", (int)first, (int)last, (int)pivot_index,
                    *(int *)pivot_value);
    bsal_vector_helper_print_int(self);
    printf("\n");
#endif

    return pivot_index;
}

int64_t bsal_vector_helper_select_pivot(struct bsal_vector *self,
                int64_t first, int64_t last, bsal_compare_fn_t compare)
{

    int64_t middle;

    void *first_value;
    void *last_value;
    void *middle_value;

    middle = first + (last - first) / 2;

    first_value = bsal_vector_at(self, first);
    last_value = bsal_vector_at(self, last);
    middle_value = bsal_vector_at(self, middle);

    if (compare(first_value, middle_value) <= 0 && compare(middle_value, last_value) <= 0) {
        return middle;

    } else if (compare(middle_value, first_value) <= 0 && compare(first_value, last_value) <= 0) {
        return first;
    }

    return last;
}

void bsal_vector_helper_swap(struct bsal_vector *self,
                int64_t index1, int64_t index2)
{
    void *value1;
    void *value2;
    int i;
    char saved;
    int element_size;

    value1 = bsal_vector_at(self, index1);
    value2 = bsal_vector_at(self, index2);

    if (value1 == NULL || value2 == NULL) {
        return;
    }

    element_size = bsal_vector_element_size(self);

    /* swap 2 elements, byte by byte.
     */
    for (i = 0; i < element_size; i++) {
        saved = ((char *)value1)[i];
        ((char *)value1)[i] = ((char *)value2)[i];
        ((char *)value2)[i] = saved;
    }
}

float bsal_vector_helper_at_as_float(struct bsal_vector *self, int64_t index)
{
    float *bucket;

    bucket = (float *)bsal_vector_at(self, index);

    if (bucket == NULL) {
        return -1;
    }

    return *bucket;
}

void bsal_vector_helper_sort_float_reverse(struct bsal_vector *self)
{
    bsal_vector_helper_sort(self, bsal_vector_helper_compare_float_reverse);
}

void bsal_vector_helper_sort_float(struct bsal_vector *self)
{
    bsal_vector_helper_sort(self, bsal_vector_helper_compare_float);
}

int bsal_vector_helper_compare_float(const void *a, const void *b)
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

int bsal_vector_helper_compare_float_reverse(const void *a, const void *b)
{
    return bsal_vector_helper_compare_float(b, a);
}


