
#include "spinlock.h"

#include "atomic.h"

/*#define CORE_SPINLOCK_READS_BETWEEN_WRITE_ATTEMPT 1024*/
#define CORE_SPINLOCK_READS_BETWEEN_WRITE_ATTEMPT 8

int core_spinlock_lock_private(int *lock);

void core_spinlock_init(struct core_spinlock *self)
{
#if defined(CORE_SPINLOCK_USE_COMPARE_AND_SWAP)
    self->lock = CORE_SPINLOCK_UNLOCKED;

#elif defined(CORE_SPINLOCK_USE_SPIN_LOCK)
    pthread_spin_init(&self->lock, 0);
#elif defined(CORE_SPINLOCK_USE_MUTEX)
    pthread_mutex_init(&self->lock, NULL);
#endif
}

int core_spinlock_lock(struct core_spinlock *self)
{
#if defined(CORE_SPINLOCK_USE_COMPARE_AND_SWAP)

    return core_spinlock_lock_private(&self->lock);

#elif defined(CORE_SPINLOCK_USE_SPIN_LOCK)
    return pthread_spin_lock(&self->lock);
#elif defined(CORE_SPINLOCK_USE_MUTEX)
    return pthread_mutex_lock(&self->lock);
#endif
}

int core_spinlock_unlock(struct core_spinlock *self)
{
#if defined(CORE_SPINLOCK_USE_COMPARE_AND_SWAP)
    if (core_atomic_compare_and_swap_int(&self->lock, CORE_SPINLOCK_LOCKED, CORE_SPINLOCK_UNLOCKED) == CORE_SPINLOCK_LOCKED) {
        /* successful */
        return CORE_SPINLOCK_SUCCESS;
    }

    /* not successful
     */
    return CORE_SPINLOCK_ERROR;

#elif defined(CORE_SPINLOCK_USE_SPIN_LOCK)
    return pthread_spin_unlock(&self->lock);
#elif defined(CORE_SPINLOCK_USE_MUTEX)
    return pthread_mutex_unlock(&self->lock);
#endif
}

int core_spinlock_trylock(struct core_spinlock *self)
{
#if defined(CORE_SPINLOCK_USE_COMPARE_AND_SWAP)
    int old_value = 0;
    int new_value = 1;

    if (core_atomic_compare_and_swap_int(&self->lock, old_value, new_value) == old_value) {
        /* successful */
        return CORE_SPINLOCK_SUCCESS;
    }

    /* not successful
     */
    return CORE_SPINLOCK_ERROR;

#elif defined(CORE_SPINLOCK_USE_SPIN_LOCK)
    return pthread_spin_trylock(&self->lock);
#elif defined(CORE_SPINLOCK_USE_MUTEX)
    return pthread_mutex_trylock(&self->lock);
#endif
}

void core_spinlock_destroy(struct core_spinlock *self)
{
#if defined(CORE_SPINLOCK_USE_COMPARE_AND_SWAP)
    self->lock = CORE_SPINLOCK_UNLOCKED;
#elif defined(CORE_SPINLOCK_USE_SPIN_LOCK)
    pthread_spin_destroy(&self->lock);
#elif defined(CORE_SPINLOCK_USE_MUTEX)
    pthread_mutex_destroy(&self->lock);
#endif
}

int core_spinlock_lock_private(int *lock)
{
    int reads;

    /* read the lock a number of times before actually trying to write
     * it.
     */
    while (core_atomic_compare_and_swap_int(lock, CORE_SPINLOCK_UNLOCKED, CORE_SPINLOCK_LOCKED) != CORE_SPINLOCK_UNLOCKED) {

        if (CORE_SPINLOCK_READS_BETWEEN_WRITE_ATTEMPT > 0) {
            reads = CORE_SPINLOCK_READS_BETWEEN_WRITE_ATTEMPT;

            while (*lock != CORE_SPINLOCK_UNLOCKED && reads > 0) {
                --reads;
            }
        } else {
            while (*lock != CORE_SPINLOCK_UNLOCKED) {

            }
        }
    }

    /* successful */
    return 0;
}
