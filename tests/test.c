
#include "test.h"

#include <stdio.h>
#include <string.h>

#define PADDING "7"

void bsal_test_print_result(int argc, char **argv, int passed_tests, int failed_tests)
{
    int all;
    char *test_name;
    size_t i;
    size_t start;

    all = passed_tests + failed_tests;

    test_name = argv[0];

#if 0
    printf("DEBUG %s\n", test_name);
#endif

    start = 0;
    i = 0;

    /*
     * Find the last slash
     */
    while (i < strlen(test_name)) {
        if (test_name[i] == '/') {
            start = i + 1;
        }

        ++i;
    }

    /*
     * Pad with spaces
     */

    printf("UnitTestSuite %24s", test_name + start);

    if (passed_tests > 0) {
        printf("   PASSED: %" PADDING "i", passed_tests);
    } else {
        printf("   passed: %" PADDING "i", passed_tests);
    }

    if (failed_tests > 0) {
        printf("   FAILED: %" PADDING "i", failed_tests);
    } else {
        printf("   failed: %" PADDING "i", failed_tests);
    }

    printf("   TOTAL: %" PADDING "i", all);

    printf("\n");
}
