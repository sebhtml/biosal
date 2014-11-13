
#ifndef _HELLO_ACQ_H
#define _HELLO_ACQ_H

#include <biosal.h>

#define SCRIPT_HELLO_ACQ 0xcdabeb5

struct hello_acq {
    struct core_vector initial_helloes;
};

#define HELLO_ACQ_ACTION_BASE -13000

#define ACTION_HELLO_ACQ_PEER (HELLO_ACQ_ACTION_BASE + 0)
#define ACTION_HELLO_ACQ_SELF (HELLO_ACQ_ACTION_BASE + 1)

extern struct thorium_script hello_acq_script;

#endif
