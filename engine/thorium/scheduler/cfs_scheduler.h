
#ifndef THORIUM_CFS_SCHEDULER_H
#define THORIUM_CFS_SCHEDULER_H

#include <core/system/memory_pool.h>

#include <core/structures/ordered/red_black_tree.h>

#define THORIUM_CFS_SCHEDULER 2

struct thorium_scheduler;
struct thorium_actor;

/*
 * A fair scheduler based on the ideas of
 * Linux Completely Fair Scheduler.
 *
 * \see https://www.kernel.org/doc/Documentation/scheduler/sched-design-CFS.txt
 * \see https://www.kernel.org/doc/Documentation/rbtree.txt
 */
struct thorium_cfs_scheduler {
    struct bsal_memory_pool pool;
    struct bsal_red_black_tree tree;
};

extern struct thorium_scheduler_interface thorium_cfs_scheduler_implementation;

void thorium_cfs_scheduler_init(struct thorium_scheduler *self);
void thorium_cfs_scheduler_destroy(struct thorium_scheduler *self);

int thorium_cfs_scheduler_enqueue(struct thorium_scheduler *self, struct thorium_actor *actor);
int thorium_cfs_scheduler_dequeue(struct thorium_scheduler *self, struct thorium_actor **actor);

int thorium_cfs_scheduler_size(struct thorium_scheduler *self);

void thorium_cfs_scheduler_print(struct thorium_scheduler *self, int node, int worker);

#endif
