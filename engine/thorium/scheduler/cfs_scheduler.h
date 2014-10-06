
#ifndef THORIUM_CFS_SCHEDULER_H
#define THORIUM_CFS_SCHEDULER_H

#include <core/system/memory_pool.h>

#include <core/structures/ordered/red_black_tree.h>

#define THORIUM_CFS_SCHEDULER 2

struct thorium_scheduler;
struct thorium_actor;

/*
 * A fair scheduler based on the algorithm of
 * Linux Completely Fair Scheduler (CFS).
 *
 * Actors that have messages to process are stored in a red-black tree
 * (balanced binary tree with good add() and delete() performance).
 *
 * The key used to order the nodes in the tree is the actor->virtual_runtime
 * attribute of any given actor.
 * This is the total time that an given actor used so far, in nanoseconds.
 *
 * enqueue is O(log(N))
 *   functions used:
 *    - core_red_black_tree_add()              O(log(N))
 *
 * dequeue is O(log(N))
 *   function used:
 *    - core_red_black_tree_get_lowest_key()   O(1) because of tree->cached_lowest_node
 *    - core_red_black_tree_get                O(1) because of tree->cached_last_node
 *    - core_red_black_tree_delete             O(log(N)) because of balance operations.
 *
 * For dequeue, the lowest key is always kept in cache
 * by maintaining the invariant in add() and delete().
 *
 * \see https://www.kernel.org/doc/Documentation/scheduler/sched-design-CFS.txt
 * \see https://www.kernel.org/doc/Documentation/rbtree.txt
 *
 * IBM paper:
 *
 * \see http://www.ibm.com/developerworks/library/l-completely-fair-scheduler/
 *
 * Outdated (by CFS author Ingo Molnar):
 * \see http://people.redhat.com/mingo/cfs-scheduler/sched-design-CFS.txt
 */
struct thorium_cfs_scheduler {
    struct core_memory_pool pool;
    struct core_red_black_tree tree;
};

extern struct thorium_scheduler_interface thorium_cfs_scheduler_implementation;

#endif
