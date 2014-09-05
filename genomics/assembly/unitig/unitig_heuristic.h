
#ifndef BSAL_UNITIG_HEURISTIC_H
#define BSAL_UNITIG_HEURISTIC_H

struct bsal_vector;
struct bsal_unitig_heuristic;

/*
 * A heuristic for computing unitigs.
 */
struct bsal_unitig_heuristic {

    int (*select)(struct bsal_unitig_heuristic *self,
                int current_coverage, struct bsal_vector *coverage_values);
};

void bsal_unitig_heuristic_init(struct bsal_unitig_heuristic *self);
void bsal_unitig_heuristic_destroy(struct bsal_unitig_heuristic *self);

int bsal_unitig_heuristic_select_with_flow_split(struct bsal_unitig_heuristic *self,
                int current_coverage, struct bsal_vector *coverage_values);
int bsal_unitig_heuristic_select(struct bsal_unitig_heuristic *self,
                int current_coverage, struct bsal_vector *coverage_values);

#endif
