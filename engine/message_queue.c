
#include "message_queue.h"

void bsal_message_queue_init(struct bsal_message_queue *self)
{
    bsal_ring_queue_init(&self->queue, sizeof(struct bsal_message));

#ifdef BSAL_MESSAGE_QUEUE_USE_TICKET_LOCK
    bsal_ticket_lock_init(&self->lock);
#else
    bsal_lock_init(&self->lock);
#endif
}

void bsal_message_queue_destroy(struct bsal_message_queue *self)
{
    bsal_ring_queue_destroy(&self->queue);

#ifdef BSAL_MESSAGE_QUEUE_USE_TICKET_LOCK
    bsal_ticket_lock_destroy(&self->lock);
#else
    bsal_lock_destroy(&self->lock);
#endif
}

int bsal_message_queue_enqueue(struct bsal_message_queue *self, struct bsal_message *message)
{
    int value;

    bsal_message_queue_lock(self);
    value = bsal_ring_queue_enqueue(&self->queue, message);
    bsal_message_queue_unlock(self);

    return value;
}

int bsal_message_queue_dequeue(struct bsal_message_queue *self, struct bsal_message *message)
{
    int value;

    value = 0;

    if (bsal_ring_queue_empty(&self->queue)) {
        return value;
    }

    bsal_message_queue_lock(self);
    value = bsal_ring_queue_dequeue(&self->queue, message);
    bsal_message_queue_unlock(self);

    return value;
}

int bsal_message_queue_lock(struct bsal_message_queue *self)
{
#ifdef BSAL_WORK_QUEUE_USE_TICKET_LOCK
    return bsal_ticket_lock_lock(&self->lock);
#else
    return bsal_lock_lock(&self->lock);
#endif
}

int bsal_message_queue_unlock(struct bsal_message_queue *self)
{
#ifdef BSAL_WORK_QUEUE_USE_TICKET_LOCK
    return bsal_ticket_lock_unlock(&self->lock);
#else
    return bsal_lock_unlock(&self->lock);
#endif
}

