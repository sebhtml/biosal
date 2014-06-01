
#ifndef _BUDDY_H
#define _BUDDY_H

#include <engine/actor.h>

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

struct bsal_actor_vtable buddy_vtable;

void buddy_init(struct bsal_actor *actor);
void buddy_destroy(struct bsal_actor *actor);
void buddy_receive(struct bsal_actor *actor, struct bsal_message *message);

#endif
