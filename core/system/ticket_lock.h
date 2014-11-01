
#ifndef CORE_TICKET_LOCK_H
#define CORE_TICKET_LOCK_H

#include "spinlock.h"

/*
 * A fair spin lock
 *
 * \see http://en.wikipedia.org/wiki/Ticket_lock
 * \see http://nahratzah.wordpress.com/2012/10/12/a-trivial-fair-spinlock/
 * \see http://lwn.net/Articles/267968/
 */
struct core_ticket_lock {
    struct core_spinlock lock;
    int dequeue_ticket;
    int queue_ticket;
};

void core_ticket_lock_init(struct core_ticket_lock *self);
void core_ticket_lock_destroy(struct core_ticket_lock *self);

int core_ticket_lock_lock(struct core_ticket_lock *self);
int core_ticket_lock_unlock(struct core_ticket_lock *self);
int core_ticket_lock_trylock(struct core_ticket_lock *self);

#endif
