
#ifndef THORIUM_DECISION_MAKER_H
#define THORIUM_DECISION_MAKER_H

#define THORIUM_NO_TIMEOUT (-1)

#define CHANGE_INCREASE         0
#define CHANGE_DECREASE         1
#define CHANGE_STRIDE           (50 * 1000)
#define MINIMUM_TIMEOUT         (50 * 1000)
#define TIMEOUT_INITIAL_VALUE   (500 * 1000)
#define MAXIMUM_TIMEOUT         (2 * 1000 * 1000)

#define THORIUM_DECISION_MAKER_ARRAY_SIZE (((MAXIMUM_TIMEOUT - MINIMUM_TIMEOUT) / CHANGE_STRIDE) + 1)

/**
 * Decision maker for selecting the timeout for
 * actor message aggregation.
 */
struct thorium_decision_maker {

    /*
     * Calibration data.
     */
    int output_throughputs[THORIUM_DECISION_MAKER_ARRAY_SIZE];

    int change;
};

void thorium_decision_maker_init(struct thorium_decision_maker *self);
void thorium_decision_maker_destroy(struct thorium_decision_maker *self);

void thorium_decision_maker_add_data_point(struct thorium_decision_maker *self,
                int timeout, int output_throughput);
int thorium_decision_maker_get_best_timeout(struct thorium_decision_maker *self,
                int current_timeout);
void thorium_decision_maker_print(struct thorium_decision_maker *self,
                int current_timeout);

#endif /* THORIUM_DECISION_MAKER_H */
