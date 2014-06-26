
#ifndef BSAL_MESSAGE_QUEUE_H
#define BSAL_MESSAGE_QUEUE_H

#include <structures/ring_queue.h>
#include <system/ticket_lock.h>
#include <system/lock.h>
#include "message.h"

/*
#define BSAL_MESSAGE_QUEUE_USE_TICKET_LOCK
*/

struct bsal_message_queue {
    struct bsal_ring_queue queue;

#ifdef BSAL_MESSAGE_QUEUE_USE_TICKET_LOCK
    struct bsal_ticket_lock lock;
#else
    struct bsal_lock lock;
#endif
};

void bsal_message_queue_init(struct bsal_message_queue *self);
void bsal_message_queue_destroy(struct bsal_message_queue *self);

/* returns 1 if successful
 */
int bsal_message_queue_enqueue(struct bsal_message_queue *self, struct bsal_message *message);

/* returns 1 if successful
 */
int bsal_message_queue_dequeue(struct bsal_message_queue *self, struct bsal_message *message);

int bsal_message_queue_unlock(struct bsal_message_queue *self);
int bsal_message_queue_lock(struct bsal_message_queue *self);

#endif
