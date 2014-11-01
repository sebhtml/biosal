
#include "ticket_lock.h"

#include <stdio.h>

void core_ticket_lock_init(struct core_ticket_lock *self)
{
    core_spinlock_init(&self->lock);
    self->dequeue_ticket = 0;
    self->queue_ticket = 0;
}

int core_ticket_lock_lock(struct core_ticket_lock *self)
{
    int ticket;

    /* get a ticket number
     * This is a critical section.
     */
    core_spinlock_lock(&self->lock);

    /* get ticket number and
     * remove the taken ticket from the ticket list
     */
    ticket = self->queue_ticket++;

    core_spinlock_unlock(&self->lock);

#ifdef CORE_TICKET_LOCK_DEBUG
    printf("Got ticket %d\n", ticket);
#endif

    while (ticket != self->dequeue_ticket) {
        /* spin for the win !!!
         */
    }

#ifdef CORE_TICKET_LOCK_DEBUG
    printf("My turn, ticket %d\n", ticket);
#endif

    return 0;
}

int core_ticket_lock_unlock(struct core_ticket_lock *self)
{
    /*
    return core_spinlock_unlock(&self->lock);
    */

    /* The person calls in the next customer.
     * No lock is required here.
     */

#ifdef CORE_TICKET_LOCK_DEBUG
    printf("Finished, ticket %d\n", self->dequeue_ticket);
#endif

    /*core_spinlock_lock(&self->lock);*/
    self->dequeue_ticket++;
    /*core_spinlock_unlock(&self->lock);*/

    return 0;
}

/* This is not implemented.
 * In order to work, we need to expose the ticket number to the
 * caller.
 */
int core_ticket_lock_trylock(struct core_ticket_lock *self)
{
    return -1;
}

void core_ticket_lock_destroy(struct core_ticket_lock *self)
{
    self->queue_ticket = -1;
    self->dequeue_ticket = -1;

    core_spinlock_unlock(&self->lock);

    core_spinlock_destroy(&self->lock);
}


