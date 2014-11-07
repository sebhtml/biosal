
#ifndef SOURCE_H
#define SOURCE_H

#include <biosal.h>

#define SCRIPT_SOURCE 0x7db0d530

#define OPTION_EVENT_COUNT "-ping-event-count-per-actor"
#define DEFAULT_EVENT_COUNT 40000
#define PERIOD 2000

/*
#define VERBOSITY
*/

/*
 * This actor sends ACTION_PING and receives ACTION_PING_REPLY.
 */
struct source {
    struct core_vector targets;

    int message_count;

    int leader;
    int event_count;

    int target;
};

extern struct thorium_script script_source;

#endif
