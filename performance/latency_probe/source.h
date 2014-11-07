
#ifndef SOURCE_H
#define SOURCE_H

#include <biosal.h>

#define SCRIPT_SOURCE 0x7db0d530

/*
 * This actor sends ACTION_PING.
 */
struct source {
    struct core_vector targets;

    int message_count;

    int leader;
    int event_count;

    int index;
};

extern struct thorium_script script_source;

#endif
