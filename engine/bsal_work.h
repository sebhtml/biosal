
#ifndef _BSAL_WORK_H
#define _BSAL_WORK_H

#include "bsal_actor.h"

struct bsal_work {
    struct bsal_actor *actor;
    struct bsal_message *message;
    int completed;
    int started;
};

void bsal_work_construct(struct bsal_work *work, struct bsal_actor *actor,
                struct bsal_message *message);
void bsal_work_destruct(struct bsal_work *work);

#endif
