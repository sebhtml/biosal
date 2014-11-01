
#include "ticket_spinlock.h"

#include <core/system/memory.h>
#include <core/system/atomic.h>

#include <stdio.h>

/*
#define CORE_TICKET_LOCK_DEBUG
*/

void core_ticket_spinlock_init(struct core_ticket_spinlock *self)
{
    self->dequeue_ticket = 0;
    self->enqueue_ticket = 0;
}

void core_ticket_spinlock_destroy(struct core_ticket_spinlock *self)
{
    self->enqueue_ticket = 0;
    self->dequeue_ticket = 0;
}

int core_ticket_spinlock_lock(struct core_ticket_spinlock *self)
{
    register int ticket;

    /*
     * Get a ticket number using an atomic operation.
     * This is a critical section.
     */

    /*
     * Get ticket number and
     * remove the taken ticket from the ticket list.
     * This is done by incrementing the enqueue ticket.
     */
    ticket = core_atomic_increment(&self->enqueue_ticket);

#ifdef CORE_TICKET_LOCK_DEBUG
    printf("Got ticket %d\n", ticket);
#endif

    if (ticket == self->dequeue_ticket)
        return 0;

    while (ticket != self->dequeue_ticket) {
        /*
         * Spin for the win !!!
         */
        core_atomic_spin();
    }

#ifdef CORE_TICKET_LOCK_DEBUG
    printf("My turn, ticket %d\n", ticket);
#endif

    return 0;
}

int core_ticket_spinlock_unlock(struct core_ticket_spinlock *self)
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
    /*self->dequeue_ticket++;*/

    /*
     * This could be done with an atomic operation, but at this point
     * this value is not needed at all.
     */
    core_atomic_increment(&self->dequeue_ticket);

    /*core_spinlock_unlock(&self->lock);*/

    /*
     * Do a memory fence so that the other threads see the change
     * made to memory. In particular, the new dequeue_ticket must be visible
     * for other threads.
     */
/*
    CORE_MEMORY_STORE_FENCE();
    */

    return 0;
}

/* This is not implemented.
 * In order to work, we need to expose the ticket number to the
 * caller.
 */
int core_ticket_spinlock_trylock(struct core_ticket_spinlock *self)
{
    return -1;
}


