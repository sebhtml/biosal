
#include "decision_maker.h"

#include <core/system/debugger.h>

#include <stdio.h>

#define NO_MPS (-1)

int thorium_decision_maker_get_index(struct thorium_decision_maker *self,
                int timeout);

int thorium_decision_maker_get_throughput(struct thorium_decision_maker *self,
                int timeout);

void thorium_decision_maker_set_throughput(struct thorium_decision_maker *self,
                int timeout, int throughput);

void thorium_decision_maker_init(struct thorium_decision_maker *self)
{
    int i;
    int size;

    size = THORIUM_DECISION_MAKER_ARRAY_SIZE;

    for (i = MINIMUM_TIMEOUT; i <= MAXIMUM_TIMEOUT; i += CHANGE_STRIDE) {
        thorium_decision_maker_set_throughput(self, i, NO_MPS);
    }

    self->change = CHANGE_INCREASE;
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
        /*
    printf("New timeout %d\n", timeout);
    */

    CORE_DEBUGGER_ASSERT(timeout >= MINIMUM_TIMEOUT);

    thorium_decision_maker_set_throughput(self, timeout, output_throughput);
}

int thorium_decision_maker_get_best_timeout(struct thorium_decision_maker *self,
                int current_timeout)
{
    int previous_timeout;
    int previous_throughput;
    int next_timeout;
    int next_throughput;
    int best_throughput;
    int best_timeout;
    int current_throughput;

    if (current_timeout == THORIUM_NO_TIMEOUT)
        return TIMEOUT_INITIAL_VALUE;

    current_throughput = thorium_decision_maker_get_throughput(self, current_timeout);

    /*
     * There is no throughput data on the current timeout.
     */
    if (current_timeout == NO_MPS) {
        return current_timeout;
    }

    best_timeout = current_timeout;
    best_throughput = current_throughput;

    previous_timeout = current_timeout - CHANGE_STRIDE;
    next_timeout = current_timeout + CHANGE_STRIDE;

    if (previous_timeout < MINIMUM_TIMEOUT) {
        /*
         * The timeout can only increase.
         */
        next_throughput = thorium_decision_maker_get_throughput(self, next_timeout);

        if (next_throughput == NO_MPS
                        || next_throughput > best_throughput) {
            best_timeout = next_timeout;
            best_throughput = next_throughput;
        }

    } else if (next_timeout > MAXIMUM_TIMEOUT) {
        /*
         * The timeout can only decrease.
         */

        previous_throughput = thorium_decision_maker_get_throughput(self, previous_timeout);

        if (previous_throughput == NO_MPS
                        || previous_throughput > best_throughput) {
            best_timeout = previous_timeout;
            best_throughput = previous_throughput;
        }
    } else {
        /*
         * The timeout can increase or decrease.
         */
        previous_throughput = thorium_decision_maker_get_throughput(self, previous_timeout);
        next_throughput = thorium_decision_maker_get_throughput(self, next_timeout);

        if (previous_throughput == NO_MPS) {
            /*
             * We have no MPS data for the previous timeout.
             */
            best_timeout = previous_timeout;

        } else if (next_throughput == NO_MPS) {

            /*
             * We have no MPS data for the next timeout.
             */
            best_timeout = next_timeout;

        } else {
            /*
             * We have MPS data for the previous and the next ones.
             */
            if (previous_throughput > next_throughput) {
                best_timeout = previous_timeout;
            } else {
                best_timeout = next_timeout;
            }
        }
    }

    return best_timeout;
}

void thorium_decision_maker_print(struct thorium_decision_maker *self, int current_timeout)
{
    int i;
    int size;
    int timeout_i;
    int throughput_i;
    char marker;

    size = THORIUM_DECISION_MAKER_ARRAY_SIZE;
    for (i = MINIMUM_TIMEOUT; i <= MAXIMUM_TIMEOUT; i += CHANGE_STRIDE) {
        timeout_i = i;
        throughput_i = thorium_decision_maker_get_throughput(self, timeout_i);
        marker = ' ';
        if (timeout_i == current_timeout) {
            marker = '*';
        }

        printf("decision_maker: calibration %d ns -> %d MPS %c\n",
                        timeout_i, throughput_i, marker);
    }
}

void thorium_decision_maker_set_throughput(struct thorium_decision_maker *self,
                int timeout, int throughput)
{
    int index;

    CORE_DEBUGGER_ASSERT(timeout >= MINIMUM_TIMEOUT);

    index = thorium_decision_maker_get_index(self, timeout);

    self->output_throughputs[index] = throughput;
}

int thorium_decision_maker_get_index(struct thorium_decision_maker *self,
                int timeout)
{
    int index;
    
    /*
    printf("%d\n", timeout);
    */
    CORE_DEBUGGER_ASSERT(timeout >= MINIMUM_TIMEOUT);
    CORE_DEBUGGER_ASSERT(timeout <= MAXIMUM_TIMEOUT);

    index = (timeout - MINIMUM_TIMEOUT) / CHANGE_STRIDE;

    /*
    */

    CORE_DEBUGGER_ASSERT(index >= 0);
    CORE_DEBUGGER_ASSERT(index < THORIUM_DECISION_MAKER_ARRAY_SIZE);

    return index;
}

int thorium_decision_maker_get_throughput(struct thorium_decision_maker *self,
                int timeout)
{
    int index;

    CORE_DEBUGGER_ASSERT(timeout >= MINIMUM_TIMEOUT);

    index = thorium_decision_maker_get_index(self, timeout);

    return self->output_throughputs[index];
}


