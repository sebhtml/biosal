
#include "work_queue.h"

void bsal_work_queue_init(struct bsal_work_queue *self)
{
    bsal_queue_init(&self->queue, sizeof(struct bsal_work));

#ifdef BSAL_WORK_QUEUE_USE_TICKET_LOCK
    bsal_ticket_lock_init(&self->lock);
#else
    bsal_lock_init(&self->lock);
#endif
}

void bsal_work_queue_destroy(struct bsal_work_queue *self)
{
    bsal_queue_destroy(&self->queue);
#ifdef BSAL_WORK_QUEUE_USE_TICKET_LOCK
    bsal_ticket_lock_destroy(&self->lock);
#else
    bsal_lock_destroy(&self->lock);
#endif
}

int bsal_work_queue_enqueue(struct bsal_work_queue *self, struct bsal_work *work)
{
    int value;

    bsal_work_queue_lock(self);
    value = bsal_queue_enqueue(&self->queue, work);
    bsal_work_queue_unlock(self);

    return value;
}

int bsal_work_queue_dequeue(struct bsal_work_queue *self, struct bsal_work *work)
{
    int value;

    value = 0;

    if (bsal_queue_empty(&self->queue)) {
        return value;
    }

    bsal_work_queue_lock(self);
    value = bsal_queue_dequeue(&self->queue, work);
    bsal_work_queue_unlock(self);


    return value;
}

int bsal_work_queue_size(struct bsal_work_queue *self)
{
    return bsal_queue_size(&self->queue);
}

int bsal_work_queue_lock(struct bsal_work_queue *self)
{
#ifdef BSAL_WORK_QUEUE_USE_TICKET_LOCK
    return bsal_ticket_lock_lock(&self->lock);
#else
    return bsal_lock_lock(&self->lock);
#endif
}

int bsal_work_queue_unlock(struct bsal_work_queue *self)
{
#ifdef BSAL_WORK_QUEUE_USE_TICKET_LOCK
    return bsal_ticket_lock_unlock(&self->lock);
#else
    return bsal_lock_unlock(&self->lock);
#endif
}

