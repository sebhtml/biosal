
#ifndef _SENDER_H
#define _SENDER_H

#include <biosal.h>

#define SCRIPT_SENDER 0x99a9f4e1

struct sender {
    int next;
};

#define SENDER_HELLO 0x000050c1
#define SENDER_HELLO_REPLY 0x00001716
#define SENDER_KILL 0x00007cd7
#define SENDER_SET_NEXT 0x00000f5d
#define SENDER_SET_NEXT_REPLY 0x000075c6

extern struct thorium_script sender_script;

void sender_init(struct thorium_actor *self);
void sender_destroy(struct thorium_actor *self);
void sender_receive(struct thorium_actor *self, struct thorium_message *message);

#endif
