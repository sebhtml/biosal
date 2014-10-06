
#ifndef _PROCESS_H
#define _PROCESS_H

#include <biosal.h>

#define SCRIPT_PROCESS 0x083212f2

struct process {
    int clone;
    struct core_vector initial_processes;
    int value;
    int ready;
    int cloned;
};

extern struct thorium_script process_script;

#endif
