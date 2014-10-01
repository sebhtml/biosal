
#include "unitig_heuristic.h"

#include <genomics/assembly/assembly_graph_summary.h>

#include <core/structures/vector.h>

#define REPEAT_MULTIPLIER 8

#define SUPER_CAREFUL_WITH_MULTIPLIER
#define SUPER_CAREFUL_WITH_THRESHOLD

void biosal_unitig_heuristic_init(struct biosal_unitig_heuristic *self)
{
    self->select = biosal_unitig_heuristic_select_with_flow_split;
    /*self->select = biosal_unitig_heuristic_select_highest;*/
}

void biosal_unitig_heuristic_destroy(struct biosal_unitig_heuristic *self)
{
    self->select = NULL;
}

int biosal_unitig_heuristic_select(struct biosal_unitig_heuristic *self,
                int current_coverage, struct core_vector *coverage_values)
{
    return self->select(self, current_coverage, coverage_values);
}

int biosal_unitig_heuristic_select_highest(struct biosal_unitig_heuristic *self,
                int current_coverage, struct core_vector *coverage_values)
{
    int coverage;
    int i;
    int size;
    int max_value;
    int max_index;

    max_value = max_index = -1;
    size = core_vector_size(coverage_values);

    for (i = 0; i < size; ++i) {
        coverage = core_vector_at_as_int(coverage_values, i);

        /*
         * This works because the calling code with verify symmetry anyway.
         */
        if (max_value == -1 || coverage > max_value) {
            max_value = coverage;
            max_index = i;
        }
    }

    return max_index;
}

int biosal_unitig_heuristic_select_with_flow_split(struct biosal_unitig_heuristic *self,
                int current_coverage, struct core_vector *coverage_values)
{
    int i;
    int coverage;
    int j;
    int other_coverage;
    int choice;
    int configured_divisor;
    int is_strong;
    int size;
    int threshold;
    int with_same_coverage;

    size = core_vector_size(coverage_values);

    configured_divisor = BIOSAL_MAXIMUM_DEGREE;
    threshold = current_coverage / configured_divisor;
    choice = BIOSAL_HEURISTIC_CHOICE_NONE;

    for (i = 0; i < size; i++) {

        /*
         * Check the one choice nonetheless...
         */
#if 0
        /*
         * Otherwise, if there is only one choice, return it.
         */

        if (size == 1) {
            choice = i;
            break;
        }
#endif
        coverage = core_vector_at_as_int(coverage_values, i);

        /*
         * Check if any other edge has the same coverage.
         */
        with_same_coverage = 0;

        for (j = 0; j < size; ++j) {
            other_coverage = core_vector_at_as_int(coverage_values, j);

            if (other_coverage == coverage) {
                ++with_same_coverage;
            }
        }

        if (with_same_coverage > 1) {
            continue;
        }

#ifdef SUPER_CAREFUL_WITH_MULTIPLIER
        /*
         * This change is too big to be OK.
         */
        if (coverage >= REPEAT_MULTIPLIER * current_coverage
                        || current_coverage >= REPEAT_MULTIPLIER * coverage) {
            continue;
        }
#endif

#ifdef SUPER_CAREFUL_WITH_THRESHOLD
        /*
         * This is a unitig, so it must be
         * regular. Otherwise there could be DNA misassemblies
         * at this stage.
         */
        if (!(coverage >= threshold)) {
            continue;
        }
#endif

        is_strong = 1;

        /*
         * Check out the others too and make sure that they are all weak.
         */
        for (j = 0; j < size; ++j) {

            /*
             * An edge does not compete with itself at all.
             */
            if (i == j)
                continue;
            other_coverage = core_vector_at_as_int(coverage_values, j);

            if (other_coverage >= threshold) {
                is_strong = 0;
                break;
            }
        }

        if (!is_strong) {
            continue;
        }

        /*
         * At this point, the edge satisfies everything.
         */

        choice = i;
        break;
    }

    return choice;
}
