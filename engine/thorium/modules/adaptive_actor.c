
#include "adaptive_actor.h"

#include <engine/thorium/actor.h>
#include <engine/thorium/configuration.h>

#include <core/system/debugger.h>

int thorium_actor_compute_count_small_messages_and_worker_scope(struct thorium_actor *self);
int thorium_actor_compute_count_small_messages_and_node_scope(struct thorium_actor *self);

int thorium_actor_get_suggested_actor_count(struct thorium_actor *self,
                uint32_t flags)
{
    /*
     * How many actors should be spawned per worker given that messages
     * will be small ?
     */
    if ((flags & THORIUM_ADAPTATION_FLAG_SMALL_MESSAGES)
                    && (flags & THORIUM_ADAPTATION_FLAG_SCOPE_WORKER)) {

        return thorium_actor_compute_count_small_messages_and_worker_scope(self);
    } else if ((flags & THORIUM_ADAPTATION_FLAG_SMALL_MESSAGES)
                    && (flags & THORIUM_ADAPTATION_FLAG_SCOPE_NODE)) {
        return thorium_actor_compute_count_small_messages_and_node_scope(self);

    }

    /*
     * Not implemented by default.
     */
    return -1;
}

int thorium_actor_compute_count_small_messages_and_worker_scope(struct thorium_actor *self)
{
    int worker_count_per_node;
    int count;

    /*
     * A = N / (W * R)
     *
     * see the documentation in adaptive_actor.h
     */

    worker_count_per_node = thorium_actor_node_worker_count(self);

    count = thorium_actor_compute_count_small_messages_and_node_scope(self);

    count /= worker_count_per_node;

    if (count == 0)
        count = 1;

    return count;
}

int thorium_actor_compute_count_small_messages_and_node_scope(struct thorium_actor *self)
{
    int actor_count;
    int node_count;

    node_count = thorium_actor_get_node_count(self);

    /*
     * More actors mean more messages.
     */
    actor_count = node_count * THORIUM_ACTOR_COUNT_PER_NODE_TO_NODE_COUNT_RATIO;

    /*
     * Use a minimum, always.
     */
    if (actor_count == 0) {
        actor_count = 1;
    }

    CORE_DEBUGGER_ASSERT(actor_count >= 1);

    return actor_count;
}

