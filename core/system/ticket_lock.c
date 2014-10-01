
#include "ticket_lock.h"

#include <stdio.h>

void biosal_ticket_lock_init(struct biosal_ticket_lock *self)
{
    biosal_lock_init(&self->lock);
    self->dequeue_ticket = 0;
    self->queue_ticket = 0;
}

int biosal_ticket_lock_lock(struct biosal_ticket_lock *self)
{
    int ticket;

    /* get a ticket number
     * This is a critical section.
     */
    biosal_lock_lock(&self->lock);

    /* get ticket number and
     * remove the taken ticket from the ticket list
     */
    ticket = self->queue_ticket++;

    biosal_lock_unlock(&self->lock);

#ifdef BIOSAL_TICKET_LOCK_DEBUG
    printf("Got ticket %d\n", ticket);
#endif

    while (ticket != self->dequeue_ticket) {
        /* spin for the win !!!
         */
    }

#ifdef BIOSAL_TICKET_LOCK_DEBUG
    printf("My turn, ticket %d\n", ticket);
#endif

    return 0;
}

int biosal_ticket_lock_unlock(struct biosal_ticket_lock *self)
{
    /*
    return biosal_lock_unlock(&self->lock);
    */

    /* The person calls in the next customer.
     * No lock is required here.
     */

#ifdef BIOSAL_TICKET_LOCK_DEBUG
    printf("Finished, ticket %d\n", self->dequeue_ticket);
#endif

    /*biosal_lock_lock(&self->lock);*/
    self->dequeue_ticket++;
    /*biosal_lock_unlock(&self->lock);*/

    return 0;
}

/* This is not implemented.
 * In order to work, we need to expose the ticket number to the
 * caller.
 */
int biosal_ticket_lock_trylock(struct biosal_ticket_lock *self)
{
    return -1;
}

void biosal_ticket_lock_destroy(struct biosal_ticket_lock *self)
{
    self->queue_ticket = -1;
    self->dequeue_ticket = -1;

    biosal_lock_unlock(&self->lock);

    biosal_lock_destroy(&self->lock);
}


