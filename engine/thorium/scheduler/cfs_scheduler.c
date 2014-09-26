
#include "cfs_scheduler.h"

#include "scheduler.h"

#include <engine/thorium/actor.h>

#include <core/system/debugger.h>
#include <core/system/memory_pool.h>

#include <inttypes.h>
#include <stdint.h>

#define BSAL_MEMORY_POOL_NAME_CFS_SCHEDULER 0x97b478ba

struct thorium_scheduler_interface thorium_cfs_scheduler_implementation = {
    .identifier = THORIUM_CFS_SCHEDULER,
    .name = "thorium_cfs_scheduler",
    .object_size = sizeof(struct thorium_cfs_scheduler),
    .init = thorium_cfs_scheduler_init,
    .destroy = thorium_cfs_scheduler_destroy,
    .enqueue = thorium_cfs_scheduler_enqueue,
    .dequeue = thorium_cfs_scheduler_dequeue,
    .size = thorium_cfs_scheduler_size,
    .print = thorium_cfs_scheduler_print
};

void thorium_cfs_scheduler_init(struct thorium_scheduler *self)
{
    struct thorium_cfs_scheduler *concrete_self;

    concrete_self = self->concrete_self;

    bsal_memory_pool_init(&concrete_self->pool, 131072, BSAL_MEMORY_POOL_NAME_CFS_SCHEDULER);

    bsal_red_black_tree_init(&concrete_self->tree, sizeof(uint64_t),
                    sizeof(struct thorium_actor *), &concrete_self->pool);

#if 0
    bsal_red_black_tree_set_memory_pool(&concrete_self->tree, &concrete_self->pool);
#endif

    /*
     * Use uint64_t keys for comparison instead of a memory comparison.
     */
    bsal_red_black_tree_use_uint64_t_keys(&concrete_self->tree);
}

void thorium_cfs_scheduler_destroy(struct thorium_scheduler *self)
{
    struct thorium_cfs_scheduler *concrete_self;

    concrete_self = self->concrete_self;

    bsal_red_black_tree_destroy(&concrete_self->tree);

    BSAL_DEBUGGER_ASSERT(bsal_memory_pool_profile_balance_count(&concrete_self->pool) == 0);

    bsal_memory_pool_destroy(&concrete_self->pool);
}

/*
 * TODO:
 *
 * add support for actor priority values here.
 *
 * Values are:
 *
 * - THORIUM_PRIORITY_LOW
 * - THORIUM_PRIORITY_NORMAL
 * - THORIUM_PRIORITY_HIGH
 * - THORIUM_PRIORITY_MAX
 *
 * The priority (an integer of type 'int') of an actor can be obtained with
 *
 * actor->priority
 *
 * or with
 *
 * thorium_actor_get_priority(actor)
 */
int thorium_cfs_scheduler_enqueue(struct thorium_scheduler *self, struct thorium_actor *actor)
{
    struct thorium_cfs_scheduler *concrete_self;
    uint64_t virtual_runtime;

    concrete_self = self->concrete_self;
    virtual_runtime = actor->virtual_runtime;

    bsal_red_black_tree_add_key_and_value(&concrete_self->tree, &virtual_runtime,
                    &actor);

    return 1;
}

int thorium_cfs_scheduler_dequeue(struct thorium_scheduler *self, struct thorium_actor **actor)
{
    struct thorium_cfs_scheduler *concrete_self;
    void *key;
    void *value;
    uint64_t virtual_runtime;

    key = NULL;
    value = NULL;
    virtual_runtime = 0;

    concrete_self = self->concrete_self;

    if (bsal_red_black_tree_size(&concrete_self->tree) == 0) {
        return 0;
    }

    /*
     * This is O(N)
     */
    key = bsal_red_black_tree_get_lowest_key(&concrete_self->tree);

    /*
     * This is O(1) because the tree always keep a cache of the last node.
     */
    value = bsal_red_black_tree_get(&concrete_self->tree, key);

    /*
     * This is also O(1) because the tree keeps a cache of the last node.
     */
    bsal_red_black_tree_delete(&concrete_self->tree, key);

    bsal_memory_copy(&virtual_runtime, key, sizeof(virtual_runtime));
    bsal_memory_copy(actor, value, sizeof(struct thorium_actor *));

    printf("CFS dequeue -> virtual_runtime= %" PRIu64 " actor= %d\n",
                    virtual_runtime, thorium_actor_name(*actor));

    return 1;
}

int thorium_cfs_scheduler_size(struct thorium_scheduler *self)
{
    struct thorium_cfs_scheduler *concrete_self;

    concrete_self = self->concrete_self;

    return bsal_red_black_tree_size(&concrete_self->tree);
}

void thorium_cfs_scheduler_print(struct thorium_scheduler *self, int node, int worker)
{
    struct thorium_cfs_scheduler *concrete_self;

    concrete_self = self->concrete_self;

    bsal_red_black_tree_print(&concrete_self->tree);
}


