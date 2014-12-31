
#include "adaptive_actor.h"

#include <engine/thorium/actor.h>
#include <engine/thorium/worker.h>
#include <engine/thorium/node.h>
#include <engine/thorium/configuration.h>

#include <core/system/debugger.h>

#include <math.h>

/*
 * New configuration for the many-actor approach.
 */
#define CONFIG_USE_LOG_ACTOR_COUNT_PER_NODE

int thorium_actor_compute_count_small_messages_and_worker_scope(struct thorium_actor *self);
int thorium_actor_compute_count_small_messages_and_node_scope(struct thorium_actor *self);
int thorium_actor_compute_count_small_messages_and_node_scope_many(struct thorium_actor *self);

int thorium_actor_compute_count_small_messages_and_node_scope_log(struct thorium_actor *self);

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
    /*
     * Scenarios:
     *
     * Cray XC30, 256 nodes, polytope topology (16 * 16)
     * links: 2 * 15 = 30
     *
     * With ratio: 25600 actors / node
     * With ratio and no topology: (256 * 100 / 255) = 100 messages / link
     * With ratio and topology: (256 * 100 / 30) = 853
     *
     * With constant: 2000 actors / node
     * With constant and no topology: 2000 / 255 = 7
     * With constant and topology: 2000 / 30 = 66
     *
     *
     * IBM Blue Gene/Q, 1024 nodes, polytope topology (32 * 32)
     * links: 2 * 31 = 62
     *
     * With ratio: 102400 actors / node
     * With ratio and no topology: 102400 / 1024 = 100 messages / link
     * With ratio and topology: 102400 / 62 = 1651
     *
     * With constant: 2000 actors / node
     * With constant and no topology: 2000 / 1024 = 1.95
     * With constant and topology: 2000 / 62 = 32
     */

#ifdef CONFIG_USE_LOG_ACTOR_COUNT_PER_NODE
    return thorium_actor_compute_count_small_messages_and_node_scope_log(self);
#else
    return thorium_actor_compute_count_small_messages_and_node_scope_many(self);
#endif
}

int thorium_actor_compute_count_small_messages_and_node_scope_many(struct thorium_actor *self)
{
    int actor_count;
    int node_count;
    int ratio;

    node_count = thorium_actor_get_node_count(self);

    ratio = self->node->actor_count_per_node_to_node_count_ratio_for_multiplexer;

    /*
    printf("ratio %d\n", ratio);
    */

    /*
     * More actors mean more messages.
     */
    actor_count = node_count * ratio;

    /*
     * Use a minimum, always.
     */
    if (actor_count == 0) {
        actor_count = 1;
    }

    CORE_DEBUGGER_ASSERT(actor_count >= 1);

    return actor_count;
}

int thorium_actor_compute_count_small_messages_and_node_scope_log(struct thorium_actor *self)
{
    int actor_count;
    int node_count;
    int log2_value;
    int constant;

    node_count = thorium_actor_get_node_count(self);
    constant = 400;

    if (node_count <= 16) {
        constant = 20;
    }

    /*
     * Values for calibration.
     *
     * ----------------------------------------------------
     * | constant | node_count | log2_value | actor_count |
     * ----------------------------------------------------
     * | 400      | 125        | 6.96       | 2786        |
     * | 400      | 256        | 8          | 3200        |
     * | 400      | 512        | 9          | 3600        |
     * | 400      | 1024       | 10         | 4000        |
     * ----------------------------------------------------
     */

    log2_value = log(node_count) / log(2);

    actor_count = constant * log2_value;

    if (actor_count <= 0) {
        actor_count = 1;
    }

    CORE_DEBUGGER_ASSERT(actor_count >= 1);

    return actor_count;
}

