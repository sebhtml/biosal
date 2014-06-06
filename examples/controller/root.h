
#ifndef _ROOT_H
#define _ROOT_H

#include <biosal.h>

#define ROOT_SCRIPT 0xf04a42e1

struct root {
    struct bsal_vector spawners;
    int controller;
    int events;
    int synchronized;
    int is_king;
    int ready;
};

#define ROOT_STOP_ALL 0x00005fd3

extern struct bsal_script root_script;

void root_init(struct bsal_actor *actor);
void root_destroy(struct bsal_actor *actor);
void root_receive(struct bsal_actor *actor, struct bsal_message *message);

#endif
