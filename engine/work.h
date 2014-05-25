
#ifndef _BSAL_WORK_H
#define _BSAL_WORK_H

#include "actor.h"

struct bsal_work {
    struct bsal_actor *actor;
    struct bsal_message *message;
};

void bsal_work_init(struct bsal_work *work, struct bsal_actor *actor,
                struct bsal_message *message);
void bsal_work_destroy(struct bsal_work *work);
struct bsal_message *bsal_work_message(struct bsal_work *work);
struct bsal_actor *bsal_work_actor(struct bsal_work *work);
void bsal_work_print(struct bsal_work *work);

#endif
