
#ifndef BIOSAL_TEST_H
#define BIOSAL_TEST_H

#include <core/constants.h>

#include <stdio.h>
#include <stdlib.h>

#include <stdint.h>

void biosal_test_print_result(int argc, char **argv, int passed_tests, int failed_tests);

#define BEGIN_TESTS() \
    int correct_tests; \
    int incorrect_tests; \
    correct_tests = 0; \
    incorrect_tests = 0;

#define END_TESTS() \
    biosal_test_print_result(argc, argv, correct_tests, incorrect_tests);

#define TEST_INT_IS_LOWER_THAN(a, b) \
if ((a) < (b)) { \
    correct_tests++; \
} else { \
    incorrect_tests++; \
    printf("Error File: %s, Function: %s, Line: %i\n", __FILE__, __func__, __LINE__); \
}

#define TEST_INT_IS_LOWER_THAN_OR_EQUAL(a, b) \
if ((a) <= (b)) { \
    correct_tests++; \
} else { \
    incorrect_tests++; \
    printf("Error File: %s, Function: %s, Line: %i\n", __FILE__, __func__, __LINE__); \
}

#define TEST_INT_IS_GREATER_THAN(a, b) \
if ((a) > (b)) { \
    correct_tests++; \
} else { \
    incorrect_tests++; \
    printf("Error File: %s, Function: %s, Line: %i\n", __FILE__, __func__, __LINE__); \
}

#define TEST_INT_IS_GREATER_THAN_OR_EQUAL(a, b) \
if ((a) >= (b)) { \
    correct_tests++; \
} else { \
    incorrect_tests++; \
    printf("Error File: %s, Function: %s, Line: %i\n", __FILE__, __func__, __LINE__); \
}

#define TEST_BOOLEAN_EQUALS(a, b) \
if (( a && b ) || ( !a && !b)) { \
    correct_tests++; \
} else { \
    incorrect_tests++; \
    printf("Error File: %s, Function: %s, Line: %i\n", __FILE__, __func__, __LINE__); \
}

#define TEST_INT_EQUALS(a, b) \
if ((int)a == (int)b) { \
    correct_tests++; \
} else { \
    incorrect_tests++; \
    printf("Actual %d Expected %d\n", (int)a, (int)b); \
    printf("Error File: %s, Function: %s, Line: %i\n", __FILE__, __func__, __LINE__); \
}

#define TEST_INT_NOT_EQUALS(a, b) \
if (a != b) { \
    correct_tests++; \
} else { \
    incorrect_tests++; \
    printf("Error File: %s, Function: %s, Line: %i\n", __FILE__, __func__, __LINE__); \
}

#define TEST_POINTER_EQUALS(a, b) \
if (a == b) { \
    correct_tests++; \
} else { \
    incorrect_tests++; \
    printf("Error File: %s, Function: %s, Line: %i\n", __FILE__, __func__, __LINE__); \
}

#define TEST_POINTER_NOT_EQUALS(a, b) \
if (a != b) { \
    correct_tests++; \
} else { \
    incorrect_tests++; \
    printf("Error File: %s, Function: %s, Line: %i\n", __FILE__, __func__, __LINE__); \
}

#define TEST_UINT64_T_EQUALS(a, b) \
if (a == b) { \
    correct_tests++; \
} else { \
    incorrect_tests++; \
    printf("Error File: %s, Function: %s, Line: %i\n", __FILE__, __func__, __LINE__); \
}



#endif
