
#ifndef PROCESS_H
#define PROCESS_H

#include <biosal.h>

#define SCRIPT_LATENCY_PROCESS 0xb4a99051

/*
 * A ping-pong actor for testing and debugging
 * Thorium and hardware platforms.
 */
struct process {
    struct core_vector initial_actors;
    struct core_vector children;
    struct core_vector target_children;
    struct core_vector actors;
    struct core_vector targets;

    struct core_timer timer;
    int size;

    int message_count;
    int completed;

    int leader;
    int event_count;

    int index;

    int mode;
};

extern struct thorium_script process_script;

#endif
