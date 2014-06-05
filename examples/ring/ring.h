
#ifndef _RING_H
#define _RING_H

#include <biosal.h>

#define RING_SCRIPT 0x88ad8b92

struct ring {
    struct bsal_vector spawners;
    int senders;
    int first;
    int last;
    int ready;
    int step;
};

#define RING_READY 0x000067db
#define RING_KILL 0x00004cfe
#define RING_SET_NEXT 0x00003833
#define RING_PUSH_NEXT 0x0000153c

extern struct bsal_script ring_script;

void ring_init(struct bsal_actor *actor);
void ring_destroy(struct bsal_actor *actor);
void ring_receive(struct bsal_actor *actor, struct bsal_message *message);

#endif
