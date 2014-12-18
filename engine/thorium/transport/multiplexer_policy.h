
#ifndef THORIUM_MULTIPLEXER_POLICY_H
#define THORIUM_MULTIPLEXER_POLICY_H

#include <core/structures/set.h>

#define THORIUM_DYNAMIC_TIMEOUT (-789)

/*
#define THORIUM_MULTIPLEXER_USE_ACTIONS_TO_SKIP
#define THORIUM_MULTIPLEXER_USE_ACTIONS_TO_MULTIPLEX
*/

/*
 * Separate policy from mechanism
 *
 * \see http://en.wikipedia.org/wiki/Separation_of_mechanism_and_policy
 */
struct thorium_multiplexer_policy {
    int degree_of_aggregation_limit;
    int threshold_buffer_size_in_bytes;
    int threshold_time_in_nanoseconds;
    struct core_set actions_to_skip;
    struct core_set actions_to_multiplex;
    int disabled;
    int minimum_node_count;
};

void thorium_multiplexer_policy_init(struct thorium_multiplexer_policy *self);
void thorium_multiplexer_policy_destroy(struct thorium_multiplexer_policy *self);

int thorium_multiplexer_policy_is_action_to_skip(struct thorium_multiplexer_policy *self, int action);
int thorium_multiplexer_policy_is_disabled(struct thorium_multiplexer_policy *self);
int thorium_multiplexer_policy_size_threshold(struct thorium_multiplexer_policy *self);
int thorium_multiplexer_policy_time_threshold(struct thorium_multiplexer_policy *self);
int thorium_multiplexer_policy_minimum_node_count(struct thorium_multiplexer_policy *self);

#endif
