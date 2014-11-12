
#ifndef _HELLO_H
#define _HELLO_H

#include <biosal.h>

#define SCRIPT_HELLO 0xfbcedbb5

struct hello_acq {
    struct core_vector initial_helloes;
};

extern struct thorium_script hello_acq_script;

#endif
