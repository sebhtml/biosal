
#ifndef BIOSAL_TICKET_LOCK_H
#define BIOSAL_TICKET_LOCK_H

#include "lock.h"

/*
 * A fair spin lock
 *
 * \see http://en.wikipedia.org/wiki/Ticket_lock
 * \see http://nahratzah.wordpress.com/2012/10/12/a-trivial-fair-spinlock/
 * \see http://lwn.net/Articles/267968/
 */
struct biosal_ticket_lock {
    struct biosal_lock lock;
    int dequeue_ticket;
    int queue_ticket;
};

void biosal_ticket_lock_init(struct biosal_ticket_lock *self);
int biosal_ticket_lock_lock(struct biosal_ticket_lock *self);
int biosal_ticket_lock_unlock(struct biosal_ticket_lock *self);
int biosal_ticket_lock_trylock(struct biosal_ticket_lock *self);
void biosal_ticket_lock_destroy(struct biosal_ticket_lock *self);

#endif
