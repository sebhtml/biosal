
#ifndef _MOCK_H
#define _MOCK_H

#include <biosal.h>

#define SCRIPT_MOCK 0x959ff8ea

struct mock {
    int value;
    int children[3];
    int remote_actor;
    int notified;
    struct core_vector spawners;
};

#define ACTION_MOCK_PREPARE_DEATH 0x00007437
#define ACTION_MOCK_NOTIFY 0x00002d45
#define ACTION_MOCK_NEW_CONTACTS 0x00003f75
#define ACTION_MOCK_NEW_CONTACTS_REPLY 0x000071a1

extern struct thorium_script mock_script;

#endif
