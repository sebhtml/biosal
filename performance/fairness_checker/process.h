
#ifndef PROCESS_H
#define PROCESS_H

#include <biosal.h>

#define SCRIPT_FAIRNESS_PROCESS 0x01412af4

/*
 * Report percentiles for point-to-point for interval duration
 * between any pair of consecutive ACTION_PING_REPLY events.
 */
struct process {
    struct core_vector times;
    struct core_vector actors;
    struct core_timer timer;
    int ready;
    int received_ping_events;
};

extern struct thorium_script process_script;

#endif
