
#ifndef _MOCK_H
#define _MOCK_H

#include <biosal.h>

#define MOCK_SCRIPT 0x959ff8ea

struct mock {
    int value;
    int children[3];
    int remote_actor;
    int notified;
};

#define MOCK_DIE 0x00006917
#define MOCK_PREPARE_DEATH 0x00007437
#define MOCK_NOTIFY 0x00002d45
#define MOCK_NEW_CONTACTS 0x00003f75
#define MOCK_NEW_CONTACTS_OK 0x000071a1

extern struct bsal_script mock_script;

void mock_init(struct bsal_actor *actor);
void mock_destroy(struct bsal_actor *actor);
void mock_receive(struct bsal_actor *actor, struct bsal_message *message);

void mock_start(struct bsal_actor *actor, struct bsal_message *message);
void mock_spawn_children(struct bsal_actor *actor);
void mock_die(struct bsal_actor *actor, struct bsal_message *message);

void mock_add_contacts(struct bsal_actor *actor, struct bsal_message *message);
void mock_share(struct bsal_actor *actor, struct bsal_message *message);

#endif
