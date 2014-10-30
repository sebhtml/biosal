
#ifndef PROCESS_H
#define PROCESS_H

#include <biosal.h>

#define SCRIPT_LATENCY_PROCESS 0xb4a99051

/*
 */
struct process {
    struct core_timer timer;
    struct core_vector initial_actors;
    struct core_vector children;
    struct core_vector actors;
    int size;

    int message_count;
    int completed;

    int leader;
};

extern struct thorium_script process_script;

#endif
