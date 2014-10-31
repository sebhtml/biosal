
#ifndef PROCESS_H
#define PROCESS_H

#include <biosal.h>

#define SCRIPT_BINOMIAL_TREE_PROCESS 0x4dbd5a69

/*
 */
struct process
{
    struct core_vector initial_actors;

    int completed;
};

extern struct thorium_script process_script;

#endif
