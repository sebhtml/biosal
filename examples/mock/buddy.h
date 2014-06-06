
#ifndef _BUDDY_H
#define _BUDDY_H

#include <biosal.h>

#define BUDDY_SCRIPT 0xd0c6b654

struct buddy {
    int received;
};

#define BUDDY_HELLO 0x000028b6
#define BUDDY_HELLO_REPLY 0x00003819
#define BUDDY_BOOT 0x0000265b
#define BUDDY_BOOT_REPLY 0x00006d65

extern struct bsal_script buddy_script;

void buddy_init(struct bsal_actor *actor);
void buddy_destroy(struct bsal_actor *actor);
void buddy_receive(struct bsal_actor *actor, struct bsal_message *message);

#endif
