
#ifndef CORE_STATISTICS_H
#define CORE_STATISTICS_H

struct core_vector;
struct core_map;

double core_statistics_get_mean_int(struct core_vector *vector);
int core_statistics_get_median_int(struct core_vector *vector);
double core_statistics_get_standard_deviation_int(struct core_vector *vector);

int core_statistics_get_percentile_int(struct core_vector *vector, int percentile);
void core_statistics_print_percentiles_int(struct core_vector *vector);

float core_statistics_get_percentile_float(struct core_vector *vector, int percentile);
void core_statistics_print_percentiles_float(struct core_vector *vector);

int core_statistics_get_percentile_int_map(struct core_map *map, int percentile);

#endif
