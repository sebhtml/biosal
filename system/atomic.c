
#include "atomic.h"

/*
 * These are mock implementations that don't work
 */

int bsal_atomic_read_int_mock(int *pointer)
{
    return *pointer;
}

int bsal_atomic_compare_and_swap_int_mock(int *pointer, int old_value, int new_value)
{
    if (*pointer != old_value) {
        return *pointer;
    }

    *pointer = new_value;

    return old_value;
}
