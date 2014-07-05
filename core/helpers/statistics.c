
#include "statistics.h"

#include "vector_helper.h"

#include <math.h>
#include <stdio.h>

double bsal_statistics_get_mean_int(struct bsal_vector *vector)
{
    double sum;
    int i;
    int count;

    count = bsal_vector_size(vector);

    if (count == 0) {
        return 0;
    }

    sum = 0;

    for (i = 0; i < count; i++) {
        sum += bsal_vector_helper_at_as_int(vector, i);
    }

    return sum / count;
}

double bsal_statistics_get_standard_deviation_int(struct bsal_vector *vector)
{
    double mean;
    double sum;
    int i;
    int count;
    double difference;

    count = bsal_vector_size(vector);

    if (count == 0) {
        return 0;
    }

    mean = bsal_statistics_get_mean_int(vector);

    sum = 0;

    for (i = 0; i < count; i++) {
        difference = bsal_vector_helper_at_as_int(vector, i) - mean;

        sum += difference * difference;
    }

    return sqrt(sum) / count;
}

int bsal_statistics_get_median_int(struct bsal_vector *vector)
{
    int count;

    count = bsal_vector_size(vector);

    if (count == 0) {
        return 0;
    }

    return bsal_vector_helper_at_as_int(vector, count / 2);
}

int bsal_statistics_get_percentile_int(struct bsal_vector *vector, int p)
{
    int index;
    int size;

    size = bsal_vector_size(vector);

    if (size == 0) {
        return 0;
    }

    index = ((0.0 + p) / 100 ) * (size - 1);

    /*
    printf("percentile %d size %d\n", p, size);
    bsal_vector_helper_print_int(vector);
    */

    return bsal_vector_helper_at_as_int(vector, index);
}

void bsal_statistics_get_print_percentiles_int(struct bsal_vector *vector)
{
    int percentile_0;
    int percentile_5;
    int percentile_25;
    int percentile_40;
    int percentile_50;
    int percentile_60;
    int percentile_75;
    int percentile_95;
    int percentile_100;

    int size;

    size = bsal_vector_size(vector);

    if (size == 0) {
        printf("empty\n");
        return;
    }

    percentile_0 = bsal_statistics_get_percentile_int(vector, 0);
    percentile_5 = bsal_statistics_get_percentile_int(vector, 5);
    percentile_25 = bsal_statistics_get_percentile_int(vector, 25);
    percentile_40 = bsal_statistics_get_percentile_int(vector, 40);
    percentile_50 = bsal_statistics_get_percentile_int(vector, 50);
    percentile_60 = bsal_statistics_get_percentile_int(vector, 60);
    percentile_75 = bsal_statistics_get_percentile_int(vector, 75);
    percentile_95 = bsal_statistics_get_percentile_int(vector, 95);
    percentile_100 = bsal_statistics_get_percentile_int(vector, 100);

    printf("%d%%: %d, %d%%: %d, %d%%: %d, %d%%: %d, %d%%: %d, %d%%: %d, %d%%: %d, %d%%: %d, %d%%: %d\n",
                    0, percentile_0,
                    5, percentile_5,
                    25, percentile_25,
                    40, percentile_40,
                    50, percentile_50,
                    60, percentile_60,
                    75, percentile_75,
                    95, percentile_95,
                    100, percentile_100);
}
