
#include "active_message_limit.h"

#include <engine/thorium/actor.h>

/*
 * This constant can be used by actors as a hint that they are
 * part of a big job.
 *
 * For example, an actor may choose to send more messages onto the
 * actor messaging fabric if there are a lot of nodes. The
 * motivation is that more nodes provide more distributed memory.
 *
 * For each <THORIUM_ACTOR_EXPOSE_ACQUAINTANCE_VECTOR>, an actor is allowed
 * to generate 1 active messages.
 *
 * The value is completely arbitrary.
 */

#ifdef __bgq__
#define THORIUM_NODE_COUNT_PER_ACTIVE_MESSAGE 256
#else
#define THORIUM_NODE_COUNT_PER_ACTIVE_MESSAGE 256
#endif

int thorium_actor_active_message_limit(struct thorium_actor *self)
{
    int node_count;
    int bonus;
    int value;

    value = 1;

    node_count = thorium_actor_get_node_count(self);

    /*
     * For a single-node job. more active messages are permitted.
     * Increase the active message count if running on one node.
     */
    if (node_count == 1) {
        /*
         * If the thing runs on one single node, there is no transport
         * calls so everything should be regular.
         */

        value += 4;
    }

    /*
     * Add bonus damage with the upgrade for big jobs.
     */
    bonus = node_count / THORIUM_NODE_COUNT_PER_ACTIVE_MESSAGE;

    value += bonus;

    return value;
}
