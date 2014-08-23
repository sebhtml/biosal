
#ifndef _TABLE_H
#define _TABLE_H

#include <biosal.h>

#define SCRIPT_TABLE 0x53f9c43c

struct table {
    int received;
    int done;
    struct bsal_vector spawners;
};

#define ACTION_TABLE_DIE 0x00003391
#define ACTION_TABLE_DIE2 0x000005e0
#define ACTION_TABLE_NOTIFY 0x000026ea

extern struct thorium_script table_script;

void table_init(struct thorium_actor *self);
void table_destroy(struct thorium_actor *self);
void table_receive(struct thorium_actor *self, struct thorium_message *message);

#endif
