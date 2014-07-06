
#ifndef BSAL_STATISTICS_H
#define BSAL_STATISTICS_H

#include <core/structures/vector.h>

double bsal_statistics_get_mean_int(struct bsal_vector *vector);
int bsal_statistics_get_median_int(struct bsal_vector *vector);
double bsal_statistics_get_standard_deviation_int(struct bsal_vector *vector);

int bsal_statistics_get_percentile_int(struct bsal_vector *vector, int percentile);
void bsal_statistics_get_print_percentiles_int(struct bsal_vector *vector);

float bsal_statistics_get_percentile_float(struct bsal_vector *vector, int percentile);
void bsal_statistics_get_print_percentiles_float(struct bsal_vector *vector);

#endif
