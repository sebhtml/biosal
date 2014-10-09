
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

/*
 * Blue Gene/Q only has 16 GiB per node
 *
 * 512 -> 1 + 2
 * 1024 -> 1 + 4
 * 1546 -> 1 + 4
 * 2048 -> 1 + 4
 */

#define THORIUM_NODE_COUNT_PER_ACTIVE_MESSAGE_PORTABLE 256

#ifdef __DISABLEbgq__
#define THORIUM_NODE_COUNT_PER_ACTIVE_MESSAGE THORIUM_NODE_COUNT_PER_ACTIVE_MESSAGE_PORTABLE

/*
 * Typically, Cray systems have more memory per node
 */
#elif defined(_CR_DISABLE_AYC)
#define THORIUM_NODE_COUNT_PER_ACTIVE_MESSAGE THORIUM_NODE_COUNT_PER_ACTIVE_MESSAGE_PORTABLE

#else
#define THORIUM_NODE_COUNT_PER_ACTIVE_MESSAGE THORIUM_NODE_COUNT_PER_ACTIVE_MESSAGE_PORTABLE

#endif

/*
#define USE_MAXIMUM
*/

int thorium_actor_active_message_limit_adaptive(struct thorium_actor *self);

int thorium_actor_active_message_limit(struct thorium_actor *self)
{
    return 1;
}

int thorium_actor_active_message_limit_adaptive(struct thorium_actor *self)
{
    int node_count;
    int bonus;
    int value;

#ifdef USE_MAXIMUM
    int maximum;
#endif

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

#ifdef USE_MAXIMUM

    /*
     * 4 * 256 = 1024
     */
    maximum = 6;

    if (bonus > maximum) {
        bonus = maximum;
    }
#endif

    value += bonus;

    return value;
}
