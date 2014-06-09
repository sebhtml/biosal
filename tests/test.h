
#ifndef BSAL_TEST_H
#define BSAL_TEST_H

#include <stdio.h>
#include <stdlib.h>

    /*#ifdef(__GNUC__) \ */
    /*#endif \ */

#define BEGIN_TESTS() \
    int correct_tests; \
    int incorrect_tests; \
    correct_tests = 0; \
    incorrect_tests = 0;

#define END_TESTS() \
    int all = correct_tests + incorrect_tests; \
    printf("PASS: %i/%i\nFAIL: %i/%i\n", correct_tests, all, incorrect_tests, all);

int test_int_equals(int a, int b);
int test_int_not_equals(int a, int b);

int test_pointer_equals(void *a, void *b);
int test_pointer_not_equals(void *a, void *b);

#define TEST_INT_EQUALS(a, b) \
if (test_int_equals(a, b)) { \
    correct_tests++; \
} else { \
    incorrect_tests++; \
    printf("Error File: %s, Function: %s, Line: %i\n", __FILE__, __func__, __LINE__); \
}

#define TEST_INT_NOT_EQUALS(a, b) \
if (test_int_not_equals(a, b)) { \
    correct_tests++; \
} else { \
    incorrect_tests++; \
    printf("Error File: %s, Function: %s, Line: %i\n", __FILE__, __func__, __LINE__); \
}

#define TEST_POINTER_EQUALS(a, b) \
if (test_pointer_equals(a, b)) { \
    correct_tests++; \
} else { \
    incorrect_tests++; \
    printf("Error File: %s, Function: %s, Line: %i\n", __FILE__, __func__, __LINE__); \
}

#define TEST_POINTER_NOT_EQUALS(a, b) \
if (test_pointer_not_equals(a, b)) { \
    correct_tests++; \
} else { \
    incorrect_tests++; \
    printf("Error File: %s, Function: %s, Line: %i\n", __FILE__, __func__, __LINE__); \
}

#endif
