
#include "multiplexer_policy.h"

#include "message_multiplexer.h"

#include <core/helpers/set_helper.h>

#include <engine/thorium/node.h>
#include <engine/thorium/actor.h>

/*
 * Size threshold.
 *
 * The current value is 8 KiB.
 */
#define THORIUM_MESSAGE_MULTIPLEXER_SIZE_THRESHOLD_IN_BYTES (8 * 1024)

/*
 * Time threshold in microseconds.
 *
 * There are 1 000 ms in 1 second
 * There are 1 000 000 us in 1 second.
 * There are 1 000 000 000 ns in 1 second.
 */
#define TIMEOUT_IN_MICRO_SECONDS 0
#define THORIUM_MESSAGE_MULTIPLEXER_TIME_THRESHOLD_IN_NANOSECONDS ( TIMEOUT_IN_MICRO_SECONDS * 1000)

void thorium_multiplexer_policy_init(struct thorium_multiplexer_policy *self)
{
    self->threshold_buffer_size_in_bytes = THORIUM_MESSAGE_MULTIPLEXER_SIZE_THRESHOLD_IN_BYTES;
    /*self->threshold_time_in_nanoseconds = THORIUM_DYNAMIC_TIMEOUT;*/
    self->threshold_time_in_nanoseconds = THORIUM_MESSAGE_MULTIPLEXER_TIME_THRESHOLD_IN_NANOSECONDS;

    biosal_set_init(&self->actions_to_skip, sizeof(int));

    /*
     * We don't want to slow down things so the following actions
     * are not multiplexed.
     */

    biosal_set_add_int(&self->actions_to_skip, ACTION_MULTIPLEXER_MESSAGE);
    biosal_set_add_int(&self->actions_to_skip, ACTION_THORIUM_NODE_START);
    biosal_set_add_int(&self->actions_to_skip, ACTION_THORIUM_NODE_ADD_INITIAL_ACTOR);
    biosal_set_add_int(&self->actions_to_skip, ACTION_THORIUM_NODE_ADD_INITIAL_ACTORS);
    biosal_set_add_int(&self->actions_to_skip, ACTION_THORIUM_NODE_ADD_INITIAL_ACTORS_REPLY);
    biosal_set_add_int(&self->actions_to_skip, ACTION_SPAWN);
    biosal_set_add_int(&self->actions_to_skip, ACTION_SPAWN_REPLY);

    self->disabled = 1;
}

void thorium_multiplexer_policy_destroy(struct thorium_multiplexer_policy *self)
{
    biosal_set_destroy(&self->actions_to_skip);

    self->threshold_buffer_size_in_bytes = -1;
    self->threshold_time_in_nanoseconds = -1;
}

int thorium_multiplexer_policy_is_action_to_skip(struct thorium_multiplexer_policy *self, int action)
{
    return biosal_set_find(&self->actions_to_skip, &action);
}

int thorium_multiplexer_policy_is_disabled(struct thorium_multiplexer_policy *self)
{
    return self->disabled;
}

int thorium_multiplexer_policy_size_threshold(struct thorium_multiplexer_policy *self)
{
    return self->threshold_buffer_size_in_bytes;
}

int thorium_multiplexer_policy_time_threshold(struct thorium_multiplexer_policy *self)
{
    return self->threshold_time_in_nanoseconds;
}


