
#ifndef BSAL_WORK_QUEUE_H
#define BSAL_WORK_QUEUE_H

#include <structures/queue.h>
#include <system/ticket_lock.h>
#include "work.h"

struct bsal_work_queue {
    struct bsal_queue queue;
    struct bsal_ticket_lock lock;
};

void bsal_work_queue_init(struct bsal_work_queue *self);
void bsal_work_queue_destroy(struct bsal_work_queue *self);

/* returns 1 if successful
 */
int bsal_work_queue_enqueue(struct bsal_work_queue *self, struct bsal_work *work);

/* returns 1 if successful
 */
int bsal_work_queue_dequeue(struct bsal_work_queue *self, struct bsal_work *work);

#endif
