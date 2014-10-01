
#include "lock.h"

#include "atomic.h"

/*#define BIOSAL_LOCK_READS_BETWEEN_WRITE_ATTEMPT 1024*/
#define BIOSAL_LOCK_READS_BETWEEN_WRITE_ATTEMPT 8

void biosal_lock_init(struct biosal_lock *self)
{
#if defined(BIOSAL_LOCK_USE_COMPARE_AND_SWAP)
    self->lock = BIOSAL_LOCK_UNLOCKED;

#elif defined(BIOSAL_LOCK_USE_SPIN_LOCK)
    pthread_spin_init(&self->lock, 0);
#elif defined(BIOSAL_LOCK_USE_MUTEX)
    pthread_mutex_init(&self->lock, NULL);
#endif
}

int biosal_lock_lock(struct biosal_lock *self)
{
#if defined(BIOSAL_LOCK_USE_COMPARE_AND_SWAP)

    return biosal_lock_lock_private(&self->lock);

#elif defined(BIOSAL_LOCK_USE_SPIN_LOCK)
    return pthread_spin_lock(&self->lock);
#elif defined(BIOSAL_LOCK_USE_MUTEX)
    return pthread_mutex_lock(&self->lock);
#endif
}

int biosal_lock_unlock(struct biosal_lock *self)
{
#if defined(BIOSAL_LOCK_USE_COMPARE_AND_SWAP)
    if (biosal_atomic_compare_and_swap_int(&self->lock, BIOSAL_LOCK_LOCKED, BIOSAL_LOCK_UNLOCKED) == BIOSAL_LOCK_LOCKED) {
        /* successful */
        return BIOSAL_LOCK_SUCCESS;
    }

    /* not successful
     */
    return BIOSAL_LOCK_ERROR;

#elif defined(BIOSAL_LOCK_USE_SPIN_LOCK)
    return pthread_spin_unlock(&self->lock);
#elif defined(BIOSAL_LOCK_USE_MUTEX)
    return pthread_mutex_unlock(&self->lock);
#endif
}

int biosal_lock_trylock(struct biosal_lock *self)
{
#if defined(BIOSAL_LOCK_USE_COMPARE_AND_SWAP)
    int old_value = 0;
    int new_value = 1;

    if (biosal_atomic_compare_and_swap_int(&self->lock, old_value, new_value) == old_value) {
        /* successful */
        return BIOSAL_LOCK_SUCCESS;
    }

    /* not successful
     */
    return BIOSAL_LOCK_ERROR;

#elif defined(BIOSAL_LOCK_USE_SPIN_LOCK)
    return pthread_spin_trylock(&self->lock);
#elif defined(BIOSAL_LOCK_USE_MUTEX)
    return pthread_mutex_trylock(&self->lock);
#endif
}

void biosal_lock_destroy(struct biosal_lock *self)
{
#if defined(BIOSAL_LOCK_USE_COMPARE_AND_SWAP)
    self->lock = BIOSAL_LOCK_UNLOCKED;
#elif defined(BIOSAL_LOCK_USE_SPIN_LOCK)
    pthread_spin_destroy(&self->lock);
#elif defined(BIOSAL_LOCK_USE_MUTEX)
    pthread_mutex_destroy(&self->lock);
#endif
}

int biosal_lock_lock_private(int *lock)
{
    int reads;

    /* read the lock a number of times before actually trying to write
     * it.
     */
    while (biosal_atomic_compare_and_swap_int(lock, BIOSAL_LOCK_UNLOCKED, BIOSAL_LOCK_LOCKED) != BIOSAL_LOCK_UNLOCKED) {

        if (BIOSAL_LOCK_READS_BETWEEN_WRITE_ATTEMPT > 0) {
            reads = BIOSAL_LOCK_READS_BETWEEN_WRITE_ATTEMPT;

            while (*lock != BIOSAL_LOCK_UNLOCKED && reads > 0) {
                --reads;
            }
        } else {
            while (*lock != BIOSAL_LOCK_UNLOCKED) {

            }
        }
    }

    /* successful */
    return 0;
}
