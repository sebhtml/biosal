
#ifndef BSAL_TICKET_LOCK_H
#define BSAL_TICKET_LOCK_H

#include "lock.h"

/*
 * A fair spin lock
 *
 * \see http://en.wikipedia.org/wiki/Ticket_lock
 * \see http://nahratzah.wordpress.com/2012/10/12/a-trivial-fair-spinlock/
 * \see http://lwn.net/Articles/267968/
 */
struct bsal_ticket_lock {
    struct bsal_lock lock;
    volatile int dequeue_ticket;
    volatile int queue_ticket;
};

void bsal_ticket_lock_init(struct bsal_ticket_lock *self);
int bsal_ticket_lock_lock(struct bsal_ticket_lock *self);
int bsal_ticket_lock_unlock(struct bsal_ticket_lock *self);
int bsal_ticket_lock_trylock(struct bsal_ticket_lock *self);
void bsal_ticket_lock_destroy(struct bsal_ticket_lock *self);

#endif
