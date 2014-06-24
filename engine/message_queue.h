
#ifndef BSAL_MESSAGE_QUEUE_H
#define BSAL_MESSAGE_QUEUE_H

#include <structures/queue.h>
#include <system/ticket_lock.h>
#include "message.h"

struct bsal_message_queue {
    struct bsal_queue queue;
    struct bsal_ticket_lock lock;
};

void bsal_message_queue_init(struct bsal_message_queue *self);
void bsal_message_queue_destroy(struct bsal_message_queue *self);

/* returns 1 if successful
 */
int bsal_message_queue_enqueue(struct bsal_message_queue *self, struct bsal_message *message);

/* returns 1 if successful
 */
int bsal_message_queue_dequeue(struct bsal_message_queue *self, struct bsal_message *message);

#endif
