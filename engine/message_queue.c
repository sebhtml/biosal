
#include "message_queue.h"

void bsal_message_queue_init(struct bsal_message_queue *self)
{
    bsal_queue_init(&self->queue, sizeof(struct bsal_message));
    bsal_ticket_lock_init(&self->lock);
}

void bsal_message_queue_destroy(struct bsal_message_queue *self)
{
    bsal_queue_destroy(&self->queue);
    bsal_ticket_lock_destroy(&self->lock);
}

int bsal_message_queue_enqueue(struct bsal_message_queue *self, struct bsal_message *message)
{
    int value;

    bsal_ticket_lock_lock(&self->lock);
    value = bsal_queue_enqueue(&self->queue, message);
    bsal_ticket_lock_unlock(&self->lock);

    return value;
}

int bsal_message_queue_dequeue(struct bsal_message_queue *self, struct bsal_message *message)
{
    int value;

    value = 0;

    if (bsal_queue_empty(&self->queue)) {
        return value;
    }

    bsal_ticket_lock_lock(&self->lock);
    value = bsal_queue_dequeue(&self->queue, message);
    bsal_ticket_lock_unlock(&self->lock);

    return value;
}


