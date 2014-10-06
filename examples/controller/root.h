
#ifndef _ROOT_H
#define _ROOT_H

#include <biosal.h>

#define SCRIPT_ROOT 0xf04a42e1

struct root {
    struct core_vector spawners;
    int controller;
    int events;
    int synchronized;
    int is_king;
    int ready;

    int manager;
};

#define ACTION_ROOT_STOP_ALL 0x00005fd3

extern struct thorium_script root_script;

#endif
