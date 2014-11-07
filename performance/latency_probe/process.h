
#ifndef PROCESS_H
#define PROCESS_H

#include <biosal.h>

#define SCRIPT_LATENCY_PROCESS 0xb4a99051

#define _LATENCY_PROBE_BASE 5000000
#define ACTION_SPAWN_TARGETS (_LATENCY_PROBE_BASE + 1)
#define ACTION_SPAWN_TARGETS_REPLY (_LATENCY_PROBE_BASE + 2)

/*
 * This actor is an initial actor. It spawns source and target
 * actors and synchronize with a leader.
 */
struct process {
    struct core_vector initial_actors;
    struct core_vector children;
    struct core_vector target_children;
    struct core_vector actors;
    struct core_vector targets;

    struct core_timer timer;
    int size;

    int completed;

    int event_count;

    int mode;
};

extern struct thorium_script process_script;

#endif
