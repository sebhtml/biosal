
#ifndef THORIUM_DECISION_MAKER_H
#define THORIUM_DECISION_MAKER_H

#define THORIUM_NO_TIMEOUT (-1)

#define THORIUM_DECISION_MAKER_ARRAY_SIZE 16

/**
 * Decision maker for selecting the timeout for
 * actor message aggregation.
 */
struct thorium_decision_maker {

    /*
     * Calibration data.
     */
    int timeout_values[THORIUM_DECISION_MAKER_ARRAY_SIZE];
    int output_throughputs[THORIUM_DECISION_MAKER_ARRAY_SIZE];

    /*
     * Current index.
     */
    int current_index;
};

void thorium_decision_maker_init(struct thorium_decision_maker *self);
void thorium_decision_maker_destroy(struct thorium_decision_maker *self);

void thorium_decision_maker_add_data_point(struct thorium_decision_maker *self,
                int timeout, int output_throughput);
int thorium_decision_maker_get_best_timeout(struct thorium_decision_maker *self);
void thorium_decision_maker_print(struct thorium_decision_maker *self);

#endif /* THORIUM_DECISION_MAKER_H */
