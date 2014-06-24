
#include "work_queue.h"

void bsal_work_queue_init(struct bsal_work_queue *self)
{
    bsal_queue_init(&self->queue, sizeof(struct bsal_work));
    bsal_ticket_lock_init(&self->lock);
}

void bsal_work_queue_destroy(struct bsal_work_queue *self)
{
    bsal_queue_destroy(&self->queue);
    bsal_ticket_lock_destroy(&self->lock);
}

int bsal_work_queue_enqueue(struct bsal_work_queue *self, struct bsal_work *work)
{
    int value;

    bsal_ticket_lock_lock(&self->lock);
    value = bsal_queue_enqueue(&self->queue, work);
    bsal_ticket_lock_unlock(&self->lock);

    return value;
}

int bsal_work_queue_dequeue(struct bsal_work_queue *self, struct bsal_work *work)
{
    int value;

    value = 0;

    if (bsal_queue_empty(&self->queue)) {
        return value;
    }

    bsal_ticket_lock_lock(&self->lock);
    value = bsal_queue_dequeue(&self->queue, work);
    bsal_ticket_lock_unlock(&self->lock);

    return value;
}


