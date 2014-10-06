
#ifndef _HELLO_H
#define _HELLO_H

#include <biosal.h>

#define SCRIPT_HELLO 0xfbcedbb5

struct hello {
    struct core_vector initial_helloes;
};

extern struct thorium_script hello_script;

#endif
