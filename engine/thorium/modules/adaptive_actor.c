
#include "adaptive_actor.h"

#include <engine/thorium/actor.h>

#include <core/system/debugger.h>

int thorium_actor_compute_count_small_messages_and_worker_scope(struct thorium_actor *self);

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
    }

    /*
     * Not implemented by default.
     */
    return -1;
}

int thorium_actor_compute_count_small_messages_and_worker_scope(struct thorium_actor *self)
{
    int worker_count_per_node;
    int actor_count_per_worker;
    int node_count;
    float ratio;

    ratio = 0.10;
    node_count = thorium_actor_get_node_count(self);
    worker_count_per_node = thorium_actor_node_worker_count(self);

    /*
     * A = N / (W * R)
     */
    actor_count_per_worker = node_count / (worker_count_per_node * ratio);

    /*
     * Use a minimum, always.
     */
    if (actor_count_per_worker == 0) {
        actor_count_per_worker = 1;
    }

    CORE_DEBUGGER_ASSERT(actor_count_per_worker >= 1);

    return actor_count_per_worker;
}

