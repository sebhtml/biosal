
#ifndef _HELLO_H
#define _HELLO_H

#include <biosal.h>

#define SCRIPT_HELLO 0xfbcedbb5

struct hello {
    struct bsal_vector initial_helloes;
};

extern struct thorium_script hello_script;

void hello_init(struct thorium_actor *self);
void hello_destroy(struct thorium_actor *self);
void hello_receive(struct thorium_actor *self, struct thorium_message *message);

#endif
