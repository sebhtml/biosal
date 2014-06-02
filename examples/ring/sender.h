
#ifndef _SENDER_H
#define _SENDER_H

#include <biosal.h>

#define SENDER_SCRIPT 0x99a9f4e1

struct sender {
    int received;
    int actors_per_node;
};

enum {
    SENDER_HELLO = 1100,
    SENDER_KILL
};

struct bsal_actor_vtable sender_vtable;

void sender_init(struct bsal_actor *actor);
void sender_destroy(struct bsal_actor *actor);
void sender_receive(struct bsal_actor *actor, struct bsal_message *message);

void sender_hello(struct bsal_actor *actor, struct bsal_message *message);
void sender_start(struct bsal_actor *actor, struct bsal_message *message);
void sender_kill_all(struct bsal_actor *actor, struct bsal_message *message);

#endif
