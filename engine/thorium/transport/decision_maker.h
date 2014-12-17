
#ifndef THORIUM_DECISION_MAKER_H
#define THORIUM_DECISION_MAKER_H

#define THORIUM_TIMEOUT_NO_VALUE (-1)

#define THORIUM_TIMEOUT_STRIDE_VALUE           (50 * 1000)
#define THORIUM_TIMEOUT_MINIMUM_VALUE         (0 * 1000)
#define THORIUM_TIMEOUT_MAXIMUM_VALUE         (2 * 1000 * 1000)
#define THORIUM_TIMEOUT_INITIAL_VALUE         (2 * 1000 * 1000)

#define THORIUM_DECISION_MAKER_ARRAY_SIZE (((THORIUM_TIMEOUT_MAXIMUM_VALUE - THORIUM_TIMEOUT_MINIMUM_VALUE) / THORIUM_TIMEOUT_STRIDE_VALUE) + 1)

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
    int selector;
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
