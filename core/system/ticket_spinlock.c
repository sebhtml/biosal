
#include "ticket_spinlock.h"

#include <core/system/atomic.h>

void core_ticket_spinlock_init(struct core_ticket_spinlock *self)
{
    self->head = 0;
    self->tail = 0;
}

void core_ticket_spinlock_destroy(struct core_ticket_spinlock *self)
{
    self->tail = 0;
    self->head = 0;
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
    ticket = core_atomic_increment(&self->tail);

#ifdef CORE_TICKET_LOCK_DEBUG
    printf("Got ticket %d\n", ticket);
#endif

    /*
     * It is our turn already.
     */
    if (ticket == self->head)
        return 0;

    while (ticket != self->head) {
        /*
         * Spin for the win !!!
         *
         * On x86, _mm_fence is used (PAUSE + a couple of NOP).
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
    printf("Finished, ticket %d\n", self->head);
#endif

    /*core_spinlock_lock(&self->lock);*/
    /*self->head++;*/

    /*
     * This could be done with an atomic operation, but at this point
     * this value is not needed at all.
     */
    core_atomic_increment(&self->head);

    /*core_spinlock_unlock(&self->lock);*/

    /*
     * Do a memory fence so that the other threads see the change
     * made to memory. In particular, the new head must be visible
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


