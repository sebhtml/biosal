
#ifndef _FRAME_H
#define _FRAME_H

#include <biosal.h>

#define FRAME_SCRIPT 0x246eaaa0

struct frame {
    int value;
    int migrated_other;
};

extern struct bsal_script frame_script;

void frame_init(struct bsal_actor *actor);
void frame_destroy(struct bsal_actor *actor);
void frame_receive(struct bsal_actor *actor, struct bsal_message *message);

#endif
