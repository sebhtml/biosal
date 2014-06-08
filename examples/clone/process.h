
#ifndef _PROCESS_H
#define _PROCESS_H

#include <biosal.h>

#define PROCESS_SCRIPT 0x083212f2

struct process {
    int clone;
    struct bsal_vector initial_processes;
    int value;
    int ready;
    int cloned;
};

extern struct bsal_script process_script;

void process_init(struct bsal_actor *actor);
void process_destroy(struct bsal_actor *actor);
void process_receive(struct bsal_actor *actor, struct bsal_message *message);

#endif
