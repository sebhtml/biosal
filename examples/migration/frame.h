
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

extern struct thorium_script frame_script;

void frame_init(struct thorium_actor *self);
void frame_destroy(struct thorium_actor *self);
void frame_receive(struct thorium_actor *self, struct thorium_message *message);

#endif
