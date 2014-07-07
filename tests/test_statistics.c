
#include <core/helpers/statistics.h>

#include "test.h"

int main(int argc, char **argv)
{
    BEGIN_TESTS();

    struct bsal_vector values;
    int i;
    int percentile_5;
    int percentile_95;
    int count_percentile_5;
    int count_percentile_95;

    bsal_vector_init(&values, sizeof(int));

    for (i = 0; i < 27; i++) {
        bsal_vector_push_back(&values, &i);
    }

    /*
    bsal_statistics_get_print_percentiles_int(&values);
    */

    percentile_5 = bsal_statistics_get_percentile_int(&values, 5);
    percentile_95 = bsal_statistics_get_percentile_int(&values, 95);

    count_percentile_5 = 0;
    count_percentile_95 = 0;

    /*
    printf("P5 %d P95 %d\n",
                    percentile_5,
                    percentile_95);
                    */

    for (i = 0; i < 27; i++) {
        if (i <= percentile_5) {
            ++count_percentile_5;
            /*
            printf("%d <= P5\n", i);
            */
        }
        if (i >= percentile_95) {
            ++count_percentile_95;
            /*
            printf("%d >= P95\n", i);
            */
        }
    }

    TEST_INT_EQUALS(count_percentile_5, count_percentile_95);

    bsal_vector_destroy(&values);

    END_TESTS();

    return 0;
}
