
#ifndef _FRAME_H
#define _FRAME_H

#include <biosal.h>

#define SCRIPT_FRAME 0x246eaaa0

struct frame {
    int value;
    int migrated_other;
    int pings;

    struct core_vector acquaintance_vector;
};

extern struct thorium_script frame_script;

#endif
