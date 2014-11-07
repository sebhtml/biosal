
#ifndef TARGET_H
#define TARGET_H

#include <biosal.h>

#define SCRIPT_LATENCY_TARGET 0xb7854b7e

/*
 * This actor receives ACTION_PING
 * and sends ACTION_PING_REPLY.
 */
struct target {
    int received;
};

extern struct thorium_script script_target;

#endif
