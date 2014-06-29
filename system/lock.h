
#ifndef BSAL_LOCK_H
#define BSAL_LOCK_H

#include <pthread.h>

#include "atomic.h"

#if defined(__linux__)
#define BSAL_LOCK_USE_SPIN_LOCK

#elif defined(__bgq__) || defined(__sparc__) || defined(__sun__) || defined(__sgi)
#define BSAL_LOCK_USE_SPIN_LOCK

#elif defined(__APPLE__) || defined(MACOSX) || defined(__unix__)
#define BSAL_LOCK_USE_MUTEX

#else
#define BSAL_LOCK_USE_MUTEX

#endif

/*
Uncomment this to force mutexes
#define BSAL_LOCK_USE_MUTEX
*/

/* Remove spinlock define if BSAL_LOCK_USE_MUTEX is defined
 * by the user
 */
#ifdef BSAL_LOCK_USE_MUTEX
#ifdef BSAL_LOCK_USE_SPIN_LOCK
#undef BSAL_LOCK_USE_SPIN_LOCK
#endif
#endif

/*
#ifdef BSAL_ATOMIC_HAS_COMPARE_AND_SWAP
#define BSAL_LOCK_USE_COMPARE_AND_SWAP
#endif
*/

struct bsal_lock {

#if defined(BSAL_LOCK_USE_COMPARE_AND_SWAP)
    int lock;
#elif defined(BSAL_LOCK_USE_SPIN_LOCK)
    pthread_spinlock_t lock;
#elif defined(BSAL_LOCK_USE_MUTEX)
    pthread_mutex_t lock;
#endif

};

void bsal_lock_init(struct bsal_lock *self);
int bsal_lock_lock(struct bsal_lock *self);
int bsal_lock_unlock(struct bsal_lock *self);
int bsal_lock_trylock(struct bsal_lock *self);
void bsal_lock_destroy(struct bsal_lock *self);

#endif
