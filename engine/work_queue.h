
#ifndef BSAL_WORK_QUEUE_H
#define BSAL_WORK_QUEUE_H

#include "work.h"

#include <structures/queue.h>
#include <system/ticket_lock.h>
#include <system/lock.h>

/*
 * Use ticket lock
#define BSAL_WORK_QUEUE_USE_TICKET_LOCK
 */

struct bsal_work_queue {
    struct bsal_queue queue;

#ifdef BSAL_WORK_QUEUE_USE_TICKET_LOCK
    struct bsal_ticket_lock lock;
#else
    struct bsal_lock lock;
#endif
};

void bsal_work_queue_init(struct bsal_work_queue *self);
void bsal_work_queue_destroy(struct bsal_work_queue *self);

/* returns 1 if successful
 */
int bsal_work_queue_enqueue(struct bsal_work_queue *self, struct bsal_work *work);

/* returns 1 if successful
 */
int bsal_work_queue_dequeue(struct bsal_work_queue *self, struct bsal_work *work);

int bsal_work_queue_size(struct bsal_work_queue *self);

int bsal_work_queue_lock(struct bsal_work_queue *self);
int bsal_work_queue_unlock(struct bsal_work_queue *self);

#endif
