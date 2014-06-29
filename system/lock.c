
#include "lock.h"

#include "atomic.h"

void bsal_lock_init(struct bsal_lock *self)
{
#if defined(BSAL_LOCK_USE_COMPARE_AND_SWAP)
    self->lock = 0;
#elif defined(BSAL_LOCK_USE_SPIN_LOCK)
    pthread_spin_init(&self->lock, 0);
#elif defined(BSAL_LOCK_USE_MUTEX)
    pthread_mutex_init(&self->lock, NULL);
#endif
}

int bsal_lock_lock(struct bsal_lock *self)
{
#if defined(BSAL_LOCK_USE_COMPARE_AND_SWAP)
    int old_value = 0;
    int new_value = 1;

    while (bsal_atomic_compare_and_swap_int(&self->lock, old_value, new_value) != old_value) {
        /* spin */
    }

    /* successful */
    return 0;
#elif defined(BSAL_LOCK_USE_SPIN_LOCK)
    return pthread_spin_lock(&self->lock);
#elif defined(BSAL_LOCK_USE_MUTEX)
    return pthread_mutex_lock(&self->lock);
#endif
}

int bsal_lock_unlock(struct bsal_lock *self)
{
#if defined(BSAL_LOCK_USE_COMPARE_AND_SWAP)
    int old_value = 1;
    int new_value = 0;

    while (bsal_atomic_compare_and_swap_int(&self->lock, old_value, new_value) != old_value) {
        /* spin */
    }

    /* successful */
    return 0;

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
        return 0;
    }

    /* not successful
     */
    return -1;

#elif defined(BSAL_LOCK_USE_SPIN_LOCK)
    return pthread_spin_trylock(&self->lock);
#elif defined(BSAL_LOCK_USE_MUTEX)
    return pthread_mutex_trylock(&self->lock);
#endif
}

void bsal_lock_destroy(struct bsal_lock *self)
{
#ifdef BSAL_LOCK_USE_SPIN_LOCK
    pthread_spin_destroy(&self->lock);
#elif defined(BSAL_LOCK_USE_MUTEX)
    pthread_mutex_destroy(&self->lock);
#endif
}


