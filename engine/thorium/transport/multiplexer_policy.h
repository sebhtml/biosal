
#ifndef THORIUM_MULTIPLEXER_POLICY_H
#define THORIUM_MULTIPLEXER_POLICY_H

#include <core/structures/set.h>

#define THORIUM_DYNAMIC_TIMEOUT (-789)

/*
 * Separate policy from mechanism
 *
 * \see http://en.wikipedia.org/wiki/Separation_of_mechanism_and_policy
 */
struct thorium_multiplexer_policy {

    int threshold_buffer_size_in_bytes;
    int threshold_time_in_nanoseconds;
    struct bsal_set actions_to_skip;
    int disabled;
};

void thorium_multiplexer_policy_init(struct thorium_multiplexer_policy *self);
void thorium_multiplexer_policy_destroy(struct thorium_multiplexer_policy *self);

int thorium_multiplexer_policy_is_action_to_skip(struct thorium_multiplexer_policy *self, int action);
int thorium_multiplexer_policy_is_disabled(struct thorium_multiplexer_policy *self);
int thorium_multiplexer_policy_size_threshold(struct thorium_multiplexer_policy *self);
int thorium_multiplexer_policy_time_threshold(struct thorium_multiplexer_policy *self);

#endif
