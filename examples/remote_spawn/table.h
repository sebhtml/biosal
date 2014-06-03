
#ifndef _TABLE_H
#define _TABLE_H

#include <engine/actor.h>

#define TABLE_SCRIPT 0x53f9c43c

struct table {
    int received;
    int done;
};

enum {
    TABLE_DIE = 1100,
    TABLE_DIE2,
    TABLE_NOTIFY
};

extern struct bsal_script table_script;

void table_init(struct bsal_actor *actor);
void table_destroy(struct bsal_actor *actor);
void table_receive(struct bsal_actor *actor, struct bsal_message *message);

#endif
