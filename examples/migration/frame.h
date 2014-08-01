
#ifndef _FRAME_H
#define _FRAME_H

#include <biosal.h>

#define FRAME_SCRIPT 0x246eaaa0

struct frame {
    int value;
    int migrated_other;
    int pings;

    struct bsal_vector acquaintance_vector;
};

extern struct bsal_script frame_script;

void frame_init(struct bsal_actor *self);
void frame_destroy(struct bsal_actor *self);
void frame_receive(struct bsal_actor *self, struct bsal_message *message);

#endif
