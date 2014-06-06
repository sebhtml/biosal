
#include "lock.h"

void bsal_lock_init(struct bsal_lock *self)
{
#ifdef BSAL_LOCK_USE_SPIN_LOCK
    pthread_spin_init(&self->lock, 0);
#elif defined(BSAL_LOCK_USE_MUTEX)
    pthread_mutex_init(&self->lock, NULL);
#endif
}

int bsal_lock_lock(struct bsal_lock *self)
{
#ifdef BSAL_LOCK_USE_SPIN_LOCK
    return pthread_spin_lock(&self->lock);
#elif defined(BSAL_LOCK_USE_MUTEX)
    return pthread_mutex_lock(&self->lock);
#endif
}

int bsal_lock_unlock(struct bsal_lock *self)
{
#ifdef BSAL_LOCK_USE_SPIN_LOCK
    return pthread_spin_unlock(&self->lock);
#elif defined(BSAL_LOCK_USE_MUTEX)
    return pthread_mutex_unlock(&self->lock);
#endif
}

int bsal_lock_trylock(struct bsal_lock *self)
{
#ifdef BSAL_LOCK_USE_SPIN_LOCK
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


