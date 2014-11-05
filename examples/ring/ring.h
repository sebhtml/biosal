
#ifndef _RING_H
#define _RING_H

#include <biosal.h>

#define SCRIPT_RING 0x88ad8b92

struct ring {
    struct core_vector spawners;
    int senders;
    int first;
    int last;
    int ready_rings;
    int ready_senders;
    int step;
    int spawned_senders;
    int previous;
};

/* see notes in actor.h about how we number internal Thorium engine messages */

#define RING_ACTION_BASE -2000

#define ACTION_RING_READY (RING_ACTION_BASE + 0)
#define ACTION_RING_KILL (RING_ACTION_BASE + 1)
#define ACTION_RING_SET_NEXT (RING_ACTION_BASE + 2)
#define ACTION_RING_PUSH_NEXT (RING_ACTION_BASE + 3)
#define ACTION_RING_SPAWN (RING_ACTION_BASE + 4)

extern struct thorium_script ring_script;

#endif
