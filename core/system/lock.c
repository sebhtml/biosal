
#include "lock.h"

#include "atomic.h"

/*#define CORE_LOCK_READS_BETWEEN_WRITE_ATTEMPT 1024*/
#define CORE_LOCK_READS_BETWEEN_WRITE_ATTEMPT 8

int core_lock_lock_private(int *lock);

void core_lock_init(struct core_lock *self)
{
#if defined(CORE_LOCK_USE_COMPARE_AND_SWAP)
    self->lock = CORE_LOCK_UNLOCKED;

#elif defined(CORE_LOCK_USE_SPIN_LOCK)
    pthread_spin_init(&self->lock, 0);
#elif defined(CORE_LOCK_USE_MUTEX)
    pthread_mutex_init(&self->lock, NULL);
#endif
}

int core_lock_lock(struct core_lock *self)
{
#if defined(CORE_LOCK_USE_COMPARE_AND_SWAP)

    return core_lock_lock_private(&self->lock);

#elif defined(CORE_LOCK_USE_SPIN_LOCK)
    return pthread_spin_lock(&self->lock);
#elif defined(CORE_LOCK_USE_MUTEX)
    return pthread_mutex_lock(&self->lock);
#endif
}

int core_lock_unlock(struct core_lock *self)
{
#if defined(CORE_LOCK_USE_COMPARE_AND_SWAP)
    if (core_atomic_compare_and_swap_int(&self->lock, CORE_LOCK_LOCKED, CORE_LOCK_UNLOCKED) == CORE_LOCK_LOCKED) {
        /* successful */
        return CORE_LOCK_SUCCESS;
    }

    /* not successful
     */
    return CORE_LOCK_ERROR;

#elif defined(CORE_LOCK_USE_SPIN_LOCK)
    return pthread_spin_unlock(&self->lock);
#elif defined(CORE_LOCK_USE_MUTEX)
    return pthread_mutex_unlock(&self->lock);
#endif
}

int core_lock_trylock(struct core_lock *self)
{
#if defined(CORE_LOCK_USE_COMPARE_AND_SWAP)
    int old_value = 0;
    int new_value = 1;

    if (core_atomic_compare_and_swap_int(&self->lock, old_value, new_value) == old_value) {
        /* successful */
        return CORE_LOCK_SUCCESS;
    }

    /* not successful
     */
    return CORE_LOCK_ERROR;

#elif defined(CORE_LOCK_USE_SPIN_LOCK)
    return pthread_spin_trylock(&self->lock);
#elif defined(CORE_LOCK_USE_MUTEX)
    return pthread_mutex_trylock(&self->lock);
#endif
}

void core_lock_destroy(struct core_lock *self)
{
#if defined(CORE_LOCK_USE_COMPARE_AND_SWAP)
    self->lock = CORE_LOCK_UNLOCKED;
#elif defined(CORE_LOCK_USE_SPIN_LOCK)
    pthread_spin_destroy(&self->lock);
#elif defined(CORE_LOCK_USE_MUTEX)
    pthread_mutex_destroy(&self->lock);
#endif
}

int core_lock_lock_private(int *lock)
{
    int reads;

    /* read the lock a number of times before actually trying to write
     * it.
     */
    while (core_atomic_compare_and_swap_int(lock, CORE_LOCK_UNLOCKED, CORE_LOCK_LOCKED) != CORE_LOCK_UNLOCKED) {

        if (CORE_LOCK_READS_BETWEEN_WRITE_ATTEMPT > 0) {
            reads = CORE_LOCK_READS_BETWEEN_WRITE_ATTEMPT;

            while (*lock != CORE_LOCK_UNLOCKED && reads > 0) {
                --reads;
            }
        } else {
            while (*lock != CORE_LOCK_UNLOCKED) {

            }
        }
    }

    /* successful */
    return 0;
}
