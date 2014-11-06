
#ifndef TARGET_H
#define TARGET_H

#include <biosal.h>

#define SCRIPT_LATENCY_TARGET 0xb7854b7e

struct target {

    int received;
};

extern struct thorium_script script_target;

#endif
