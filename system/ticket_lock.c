
#include "ticket_lock.h"

#include <stdio.h>

void bsal_ticket_lock_init(struct bsal_ticket_lock *self)
{
    bsal_lock_init(&self->lock);
    self->dequeue_ticket = 0;
    self->queue_ticket = 0;
}

int bsal_ticket_lock_lock(struct bsal_ticket_lock *self)
{
    int ticket;

    /* get a ticket number
     * This is a critical section.
     */
    bsal_lock_lock(&self->lock);

    /* get ticket number and
     * remove the taken ticket from the ticket list
     */
    ticket = self->queue_ticket++;

    bsal_lock_unlock(&self->lock);

#ifdef BSAL_TICKET_LOCK_DEBUG
    printf("Got ticket %d\n", ticket);
#endif

    while (ticket != self->dequeue_ticket) {
        /* spin for the win !!!
         */
    }

#ifdef BSAL_TICKET_LOCK_DEBUG
    printf("My turn, ticket %d\n", ticket);
#endif

    return 0;
}

int bsal_ticket_lock_unlock(struct bsal_ticket_lock *self)
{
    /*
    return bsal_lock_unlock(&self->lock);
    */

    /* The person calls in the next customer.
     * No lock is required here.
     */

#ifdef BSAL_TICKET_LOCK_DEBUG
    printf("Finished, ticket %d\n", self->dequeue_ticket);
#endif

    /*bsal_lock_lock(&self->lock);*/
    self->dequeue_ticket++;
    /*bsal_lock_unlock(&self->lock);*/

    return 0;
}

/* This is not implemented.
 * In order to work, we need to expose the ticket number to the
 * caller.
 */
int bsal_ticket_lock_trylock(struct bsal_ticket_lock *self)
{
    return -1;
}

void bsal_ticket_lock_destroy(struct bsal_ticket_lock *self)
{
    self->queue_ticket = -1;
    self->dequeue_ticket = -1;

    bsal_lock_unlock(&self->lock);

    bsal_lock_destroy(&self->lock);
}


