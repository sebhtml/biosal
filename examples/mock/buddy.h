
#ifndef _BUDDY_H
#define _BUDDY_H

#include <biosal.h>

#define SCRIPT_BUDDY 0xd0c6b654

struct buddy {
    int received;
};

#define ACTION_BUDDY_HELLO 0x000028b6
#define ACTION_BUDDY_HELLO_REPLY 0x00003819
#define ACTION_BUDDY_BOOT 0x0000265b
#define ACTION_BUDDY_BOOT_REPLY 0x00006d65

extern struct thorium_script buddy_script;

void buddy_init(struct thorium_actor *self);
void buddy_destroy(struct thorium_actor *self);
void buddy_receive(struct thorium_actor *self, struct thorium_message *message);

#endif
