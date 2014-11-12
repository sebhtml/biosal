
#ifndef _HELLO_ACQ_H
#define _HELLO_ACQ_H

#include <biosal.h>

#define SCRIPT_HELLO_ACQ 0xcdabeb5

struct hello_acq {
    struct core_vector initial_helloes;
};

extern struct thorium_script hello_acq_script;

#endif
