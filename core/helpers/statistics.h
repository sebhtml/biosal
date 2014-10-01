
#ifndef BIOSAL_STATISTICS_H
#define BIOSAL_STATISTICS_H

struct biosal_vector;
struct biosal_map;

double biosal_statistics_get_mean_int(struct biosal_vector *vector);
int biosal_statistics_get_median_int(struct biosal_vector *vector);
double biosal_statistics_get_standard_deviation_int(struct biosal_vector *vector);

int biosal_statistics_get_percentile_int(struct biosal_vector *vector, int percentile);
void biosal_statistics_print_percentiles_int(struct biosal_vector *vector);

float biosal_statistics_get_percentile_float(struct biosal_vector *vector, int percentile);
void biosal_statistics_print_percentiles_float(struct biosal_vector *vector);

int biosal_statistics_get_percentile_int_map(struct biosal_map *map, int percentile);

#endif
