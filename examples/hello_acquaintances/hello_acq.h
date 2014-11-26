
#ifndef _HELLO_ACQ_H
#define _HELLO_ACQ_H

#include <biosal.h>

#define SCRIPT_HELLO_ACQ 0xcdabeb5

struct hello_acq {
    struct core_vector initial_helloes;
    struct core_vector actors_to_greet;
};

#define HELLO_ACQ_ACTION_BASE -13000

#define ACTION_HELLO_ACQ_PEER (HELLO_ACQ_ACTION_BASE + 0)
#define ACTION_HELLO_ACQ_SELF (HELLO_ACQ_ACTION_BASE + 1)
#define ACTION_HELLO_GREET_OTHERS (HELLO_ACQ_ACTION_BASE + 2)
#define ACTION_HELLO_DONE_GREETING_ALL (HELLO_ACQ_ACTION_BASE + 3)
#define ACTION_HELLO_PEER_DONE (HELLO_ACQ_ACTION_BASE + 4)

extern struct thorium_script hello_acq_script;

#endif
