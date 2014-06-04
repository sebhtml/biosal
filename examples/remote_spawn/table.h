
#ifndef _TABLE_H
#define _TABLE_H

#include <biosal.h>

#define TABLE_SCRIPT 0x53f9c43c

struct table {
    int received;
    int done;
};

#define TABLE_DIE 0x00003391
#define TABLE_DIE2 0x000005e0
#define TABLE_NOTIFY 0x000026ea

extern struct bsal_script table_script;

void table_init(struct bsal_actor *actor);
void table_destroy(struct bsal_actor *actor);
void table_receive(struct bsal_actor *actor, struct bsal_message *message);

#endif
