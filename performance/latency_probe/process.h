
#ifndef PROCESS_H
#define PROCESS_H

#include <biosal.h>

#define SCRIPT_LATENCY_PROCESS 0xb4a99051

/*
 */
struct process {
    struct core_vector initial_actors;
    struct core_vector children;
    struct core_vector actors;
    int size;
};

extern struct thorium_script process_script;

#endif
