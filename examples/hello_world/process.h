
#ifndef _PROCESS_H
#define _PROCESS_H

#include <biosal.h>

#define PROCESS_SCRIPT 0xfbcedbb5

struct process {
    struct bsal_vector initial_processes;
};

extern struct bsal_script process_script;

void process_init(struct bsal_actor *actor);
void process_destroy(struct bsal_actor *actor);
void process_receive(struct bsal_actor *actor, struct bsal_message *message);

#endif
