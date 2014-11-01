
#ifndef CORE_TICKET_SPINLOCK_H
#define CORE_TICKET_SPINLOCK_H

#include "spinlock.h"

/*
 * A fair spinlock
 *
 * \see http://en.wikipedia.org/wiki/Ticket_lock
 * \see http://nahratzah.wordpress.com/2012/10/12/a-trivial-fair-spinlock/
 * \see http://lwn.net/Articles/267968/
 */
struct core_ticket_spinlock {
    struct core_spinlock lock;
    volatile int dequeue_ticket;
    volatile int queue_ticket;
};

void core_ticket_spinlock_init(struct core_ticket_spinlock *self);
void core_ticket_spinlock_destroy(struct core_ticket_spinlock *self);

int core_ticket_spinlock_lock(struct core_ticket_spinlock *self);
int core_ticket_spinlock_unlock(struct core_ticket_spinlock *self);
int core_ticket_spinlock_trylock(struct core_ticket_spinlock *self);

#endif
