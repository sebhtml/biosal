
#ifndef _BUDDY_H
#define _BUDDY_H

#include <biosal.h>

#define BUDDY_SCRIPT 0xd0c6b654

struct buddy {
    int received;
};

enum {
    BUDDY_DIE = 1100,
    BUDDY_HELLO,
    BUDDY_HELLO_OK,
    BUDDY_BOOT,
    BUDDY_BOOT_OK
};

extern struct bsal_script buddy_script;

void buddy_init(struct bsal_actor *actor);
void buddy_destroy(struct bsal_actor *actor);
void buddy_receive(struct bsal_actor *actor, struct bsal_message *message);

#endif
