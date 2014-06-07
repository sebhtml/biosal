
#ifndef _HELLO_H
#define _HELLO_H

#include <biosal.h>

#define HELLO_SCRIPT 0xfbcedbb5

struct hello {
    struct bsal_vector initial_helloes;
};

extern struct bsal_script hello_script;

void hello_init(struct bsal_actor *actor);
void hello_destroy(struct bsal_actor *actor);
void hello_receive(struct bsal_actor *actor, struct bsal_message *message);

#endif
