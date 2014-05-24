
#include "bsal_work.h"

#include <stdlib.h>
#include <stdio.h>

void bsal_work_construct(struct bsal_work *work, struct bsal_actor *actor,
                struct bsal_message *message)
{
    work->actor = actor;
    work->message = message;
    work->completed = 0;
    work->started = 0;
}

void bsal_work_destruct(struct bsal_work *work)
{
    work->actor = NULL;
    work->message = NULL;
    work->completed = -1;
    work->started = -1;
}

struct bsal_message *bsal_work_message(struct bsal_work *work)
{
    return work->message;
}

struct bsal_actor *bsal_work_actor(struct bsal_work *work)
{
    return work->actor;
}

void bsal_work_print(struct bsal_work *work)
{
    struct bsal_actor *actor;
    struct bsal_message *message;

    printf("[bsal_work_print]\n");

    actor = bsal_work_actor(work);
    message = bsal_work_message(work);

    bsal_actor_print(actor);
    bsal_message_print(message);
}
