
#include "statistics.h"

#include "vector_helper.h"

#include <math.h>

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
