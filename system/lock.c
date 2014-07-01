
#include "lock.h"

#include "atomic.h"

/*#define BSAL_LOCK_READS_BETWEEN_WRITE_ATTEMPT 1024*/
#define BSAL_LOCK_READS_BETWEEN_WRITE_ATTEMPT 8

void bsal_lock_init(struct bsal_lock *self)
{
#if defined(BSAL_LOCK_USE_COMPARE_AND_SWAP)
    self->lock = BSAL_LOCK_UNLOCKED;

#elif defined(BSAL_LOCK_USE_SPIN_LOCK)
    pthread_spin_init(&self->lock, 0);
#elif defined(BSAL_LOCK_USE_MUTEX)
    pthread_mutex_init(&self->lock, NULL);
#endif
}

int bsal_lock_lock(struct bsal_lock *self)
{
#if defined(BSAL_LOCK_USE_COMPARE_AND_SWAP)

    return bsal_lock_lock_private(&self->lock);

#elif defined(BSAL_LOCK_USE_SPIN_LOCK)
    return pthread_spin_lock(&self->lock);
#elif defined(BSAL_LOCK_USE_MUTEX)
    return pthread_mutex_lock(&self->lock);
#endif
}

int bsal_lock_unlock(struct bsal_lock *self)
{
#if defined(BSAL_LOCK_USE_COMPARE_AND_SWAP)
    if (bsal_atomic_compare_and_swap_int(&self->lock, BSAL_LOCK_LOCKED, BSAL_LOCK_UNLOCKED) == BSAL_LOCK_LOCKED) {
        /* successful */
        return BSAL_LOCK_SUCCESS;
    }

    /* not successful
     */
    return BSAL_LOCK_ERROR;

#elif defined(BSAL_LOCK_USE_SPIN_LOCK)
    return pthread_spin_unlock(&self->lock);
#elif defined(BSAL_LOCK_USE_MUTEX)
    return pthread_mutex_unlock(&self->lock);
#endif
}

int bsal_lock_trylock(struct bsal_lock *self)
{
#if defined(BSAL_LOCK_USE_COMPARE_AND_SWAP)
    int old_value = 0;
    int new_value = 1;

    if (bsal_atomic_compare_and_swap_int(&self->lock, old_value, new_value) == old_value) {
        /* successful */
        return BSAL_LOCK_SUCCESS;
    }

    /* not successful
     */
    return BSAL_LOCK_ERROR;

#elif defined(BSAL_LOCK_USE_SPIN_LOCK)
    return pthread_spin_trylock(&self->lock);
#elif defined(BSAL_LOCK_USE_MUTEX)
    return pthread_mutex_trylock(&self->lock);
#endif
}

void bsal_lock_destroy(struct bsal_lock *self)
{
#if defined(BSAL_LOCK_USE_COMPARE_AND_SWAP)
    self->lock = BSAL_LOCK_UNLOCKED;
#elif defined(BSAL_LOCK_USE_SPIN_LOCK)
    pthread_spin_destroy(&self->lock);
#elif defined(BSAL_LOCK_USE_MUTEX)
    pthread_mutex_destroy(&self->lock);
#endif
}

int bsal_lock_lock_private(int *lock)
{
    int reads;

    /* read the lock a number of times before actually trying to write
     * it.
     */
    while (bsal_atomic_compare_and_swap_int(lock, BSAL_LOCK_UNLOCKED, BSAL_LOCK_LOCKED) != BSAL_LOCK_UNLOCKED) {

        if (BSAL_LOCK_READS_BETWEEN_WRITE_ATTEMPT > 0) {
            reads = BSAL_LOCK_READS_BETWEEN_WRITE_ATTEMPT;

            while (*lock != BSAL_LOCK_UNLOCKED && reads > 0) {
                --reads;
            }
        } else {
            while (*lock != BSAL_LOCK_UNLOCKED) {

            }
        }
    }

    /* successful */
    return 0;
}
