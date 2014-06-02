
#ifndef _MOCK_H
#define _MOCK_H

#include <engine/actor.h>

#define MOCK_SCRIPT 0x959ff8ea

struct mock {
    int value;
    int children[3];
    int remote_actor;
    int notified;
};

enum {
    MOCK_DIE = 1200, /* FIRST_TAG */
    MOCK_PREPARE_DEATH,
    MOCK_NOTIFY,
    MOCK_NEW_CONTACTS,
    MOCK_NEW_CONTACTS_OK /* LAST_TAG */
};

struct bsal_actor_vtable mock_vtable;

void mock_init(struct bsal_actor *actor);
void mock_destroy(struct bsal_actor *actor);
void mock_receive(struct bsal_actor *actor, struct bsal_message *message);

void mock_start(struct bsal_actor *actor, struct bsal_message *message);
void mock_spawn_children(struct bsal_actor *actor);
void mock_die(struct bsal_actor *actor, struct bsal_message *message);

void mock_add_contacts(struct bsal_actor *actor, struct bsal_message *message);
void mock_share(struct bsal_actor *actor, struct bsal_message *message);

#endif
