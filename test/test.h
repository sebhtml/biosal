
#ifndef _BSAL_TEST_H
#define _BSAL_TEST_H

#include <stdio.h>

int test_equal(int a, int b);

#define TEST_EQUAL(a, b) \
if(test_equal(a, b)) { \
    correct_tests++; \
} else { \
    incorrect_tests++; \
}

#define BEGIN_TESTS() \
    int correct_tests; \
    int incorrect_tests; \
    correct_tests = 0; \
    incorrect_tests = 0;

#define END_TESTS() \
    int all = correct_tests + incorrect_tests; \
    printf("Correct: %i/%i,  Incorrect: %i/%i\n", correct_tests, all, incorrect_tests, all);

#endif

