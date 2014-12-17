
#include "decision_maker.h"

#include <stdio.h>

void thorium_decision_maker_init(struct thorium_decision_maker *self)
{
    int i;
    int size;

    size = THORIUM_DECISION_MAKER_ARRAY_SIZE;

    for (i = 0; i < size; ++i) {
        self->timeout_values[i] = THORIUM_NO_TIMEOUT;
        self->output_throughputs[i] = 0;
    }

    self->current_index = 0;
}

void thorium_decision_maker_destroy(struct thorium_decision_maker *self)
{
    /*
     * Nothing to do, there is no memory allocation here.
     */
}

void thorium_decision_maker_add_data_point(struct thorium_decision_maker *self,
                int timeout, int output_throughput)
{
    int i;

    i = self->current_index;

    self->current_index++;
    if (self->current_index == THORIUM_DECISION_MAKER_ARRAY_SIZE)
        self->current_index = 0;

    self->timeout_values[i] = timeout;
    self->output_throughputs[i] = output_throughput;
}

int thorium_decision_maker_get_best_timeout(struct thorium_decision_maker *self)
{
    int i;
    int size;
    int best_throughput;
    int best_timeout;
    int timeout_i;
    int throughput_i;

    best_timeout = THORIUM_NO_TIMEOUT;
    best_throughput = 0;

    size = THORIUM_DECISION_MAKER_ARRAY_SIZE;

    for (i = 0; i < size; ++i) {
        timeout_i = self->timeout_values[i];

        /*
         * There is not enough data accumulated.
         */
        if (timeout_i == THORIUM_NO_TIMEOUT)
            return THORIUM_NO_TIMEOUT;

        throughput_i = self->output_throughputs[i];

        if (best_timeout == THORIUM_NO_TIMEOUT
                        || throughput_i > best_throughput) {
            best_timeout = timeout_i;
            best_throughput = throughput_i;
        }
    }

    return best_timeout;
}

void thorium_decision_maker_print(struct thorium_decision_maker *self)
{
    int i;
    int size;
    int timeout_i;
    int throughput_i;

    size = THORIUM_DECISION_MAKER_ARRAY_SIZE;

    for (i = 0; i < size; ++i) {
        timeout_i = self->timeout_values[i];
        throughput_i = self->output_throughputs[i];

        printf("decision_maker: calibration data [%d/%d] %d us -> %d MPS\n",
                        i, size, timeout_i, throughput_i);
    }
}
