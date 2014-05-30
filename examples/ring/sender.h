
#ifndef _SENDER_H
#define _SENDER_H

#include <biosal.h>

struct sender {
    int received;
    int actors_per_node;

    struct sender *actors;
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
