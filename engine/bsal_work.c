
#include "bsal_work.h"

#include <stdlib.h>

void bsal_work_construct(struct bsal_work *work, bsal_actor_receive_fn_t receive,
                struct bsal_actor *actor, struct bsal_message *message)
{
    work->receive = receive;
    work->actor = actor;
    work->message = message;
    work->completed = 0;
    work->started = 0;
}

void bsal_work_destruct(struct bsal_work *work)
{
    work->receive = NULL;
    work->actor = NULL;
    work->message = NULL;
    work->completed = -1;
    work->started = -1;
}
