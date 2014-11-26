
#ifndef _HELLO_ACQ_H
#define _HELLO_ACQ_H

#include <biosal.h>

#define SCRIPT_HELLO_ACQ 0xcdabeb5

struct hello_acq {
    struct core_vector initial_helloes;
    struct core_vector actors_to_greet;
};

#define HELLO_ACQ_ACTION_BASE -13000

#define ACTION_HELLO_ACQ_GREET_OTHERS (HELLO_ACQ_ACTION_BASE + 1)
#define ACTION_HELLO_ACQ_PEER (HELLO_ACQ_ACTION_BASE + 2)

extern struct thorium_script hello_acq_script;

#endif
