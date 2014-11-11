
#include "multiplexer_policy.h"

#include "message_multiplexer.h"

#include <core/helpers/set_helper.h>

#include <engine/thorium/node.h>
#include <engine/thorium/actor.h>
#include <engine/thorium/configuration.h>

void thorium_multiplexer_policy_init(struct thorium_multiplexer_policy *self)
{
    /*
     * Size threshold.
     */
    self->threshold_buffer_size_in_bytes = THORIUM_MULTIPLEXER_BUFFER_SIZE;

    /*self->threshold_time_in_nanoseconds = THORIUM_DYNAMIC_TIMEOUT;*/

    /*
     * Time threshold in microseconds.
     *
     * There are 1 000 ms in 1 second
     * There are 1 000 000 us in 1 second.
     * There are 1 000 000 000 ns in 1 second.
     */
    self->threshold_time_in_nanoseconds = THORIUM_MULTIPLEXER_TIMEOUT;

    core_set_init(&self->actions_to_skip, sizeof(int));
    core_set_init(&self->actions_to_multiplex, sizeof(int));

    /*
     * We don't want to slow down things so the following actions
     * are not multiplexed.
     */

    core_set_add_int(&self->actions_to_skip, ACTION_MULTIPLEXER_MESSAGE);
    core_set_add_int(&self->actions_to_skip, ACTION_THORIUM_NODE_START);
    core_set_add_int(&self->actions_to_skip, ACTION_THORIUM_NODE_ADD_INITIAL_ACTOR);
    core_set_add_int(&self->actions_to_skip, ACTION_THORIUM_NODE_ADD_INITIAL_ACTORS);
    core_set_add_int(&self->actions_to_skip, ACTION_THORIUM_NODE_ADD_INITIAL_ACTORS_REPLY);
    core_set_add_int(&self->actions_to_skip, ACTION_SPAWN);
    core_set_add_int(&self->actions_to_skip, ACTION_SPAWN_REPLY);

    self->disabled = 0;

    /*
     * This is the minimum number of thorium nodes
     * needed for enabling the multiplexer.
     */
    self->minimum_node_count = 1;

#if 0
    /*
     * The new model is to use explicit registration of messages to
     * multiplex.
     *
     * If no actions are registered, disable the multiplexer in the policy
     * directly.
     */
    if (core_set_empty(&self->actions_to_multiplex))
        self->disabled = 1;
#endif

    if (self->threshold_time_in_nanoseconds == 0)
        self->disabled = 1;
}

void thorium_multiplexer_policy_destroy(struct thorium_multiplexer_policy *self)
{
    core_set_destroy(&self->actions_to_skip);
    core_set_destroy(&self->actions_to_multiplex);

    self->threshold_buffer_size_in_bytes = -1;
    self->threshold_time_in_nanoseconds = -1;
}

int thorium_multiplexer_policy_is_action_to_skip(struct thorium_multiplexer_policy *self, int action)
{
    return core_set_find(&self->actions_to_skip, &action);
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

int thorium_multiplexer_policy_minimum_node_count(struct thorium_multiplexer_policy *self)
{
    return self->minimum_node_count;
}
