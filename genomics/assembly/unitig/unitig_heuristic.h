
#ifndef BIOSAL_UNITIG_HEURISTIC_H
#define BIOSAL_UNITIG_HEURISTIC_H

struct core_vector;
struct biosal_unitig_heuristic;

#define BIOSAL_HEURISTIC_CHOICE_NONE (-1)

/*
 * A heuristic for computing unitigs.
 */
struct biosal_unitig_heuristic {

    int (*select)(struct biosal_unitig_heuristic *self,
                int current_coverage, struct core_vector *coverage_values);
};

void biosal_unitig_heuristic_init(struct biosal_unitig_heuristic *self);
void biosal_unitig_heuristic_destroy(struct biosal_unitig_heuristic *self);

int biosal_unitig_heuristic_select_with_flow_split(struct biosal_unitig_heuristic *self,
                int current_coverage, struct core_vector *coverage_values);
int biosal_unitig_heuristic_select(struct biosal_unitig_heuristic *self,
                int current_coverage, struct core_vector *coverage_values);
int biosal_unitig_heuristic_select_highest(struct biosal_unitig_heuristic *self,
                int current_coverage, struct core_vector *coverage_values);

#endif
