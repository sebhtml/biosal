
#include "test.h"

int test_int_equals(int a, int b)
{
    if (a == b) {
        return 1;
    }

    return 0;
}

int test_int_not_equals(int a, int b)
{
    if (test_int_equals(a, b)) {
        return 0;
    }

    return 1;
}

int test_pointer_equals(void *a, void *b)
{
    if (a == b) {
        return 1;
    }

    return 0;
}

int test_pointer_not_equals(void *a, void *b)
{
    if (test_pointer_equals(a, b)) {
        return 0;
    }

    return 1;
}

int test_int_is_greater_than(int a, int b)
{
    if (a > b) {
        return 1;
    }

    return 0;
}
