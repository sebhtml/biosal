
#ifndef _STREAM_H
#define _STREAM_H

#include <biosal.h>

#define STREAM_SCRIPT 0xb9b19139

struct stream {
    int initial_synchronization;
};

enum {
    STREAM_DIE = 1100
};

extern struct bsal_script stream_script;

void stream_init(struct bsal_actor *actor);
void stream_destroy(struct bsal_actor *actor);
void stream_receive(struct bsal_actor *actor, struct bsal_message *message);

#endif
