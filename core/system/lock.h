
#ifndef CORE_LOCK_H
#define CORE_LOCK_H

#include <pthread.h>

#include "atomic.h"

#define CORE_LOCK_SUCCESS 0
#define CORE_LOCK_ERROR -1

#define CORE_LOCK_UNLOCKED 0
#define CORE_LOCK_LOCKED 1

#if defined(__linux__) && defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L
#define CORE_LOCK_USE_SPIN_LOCK

#elif defined(__bgq__)
#define CORE_LOCK_USE_SPIN_LOCK

#elif defined(__APPLE__) || defined(MACOSX) || defined(__unix__)
#define CORE_LOCK_USE_MUTEX

#else
#define CORE_LOCK_USE_MUTEX

#endif

/*
Uncomment this to force mutexes
#define CORE_LOCK_USE_MUTEX
*/

/* Remove spinlock define if CORE_LOCK_USE_MUTEX is defined
 * by the user
 */
#ifdef CORE_LOCK_USE_MUTEX
#ifdef CORE_LOCK_USE_SPIN_LOCK
#undef CORE_LOCK_USE_SPIN_LOCK
#endif
#endif

/*
 * Uncomment this to use the custom spinlock (which is
 * not very good.
 *
 */
/*
#ifdef CORE_ATOMIC_HAS_COMPARE_AND_SWAP
#define CORE_LOCK_USE_COMPARE_AND_SWAP
#endif
*/

struct core_lock {

#if defined(CORE_LOCK_USE_COMPARE_AND_SWAP)
    int lock;

#elif defined(CORE_LOCK_USE_SPIN_LOCK)
/*#error "SPIN LOCK"*/
    pthread_spinlock_t lock;
#elif defined(CORE_LOCK_USE_MUTEX)
    pthread_mutex_t lock;
#endif

};

void core_lock_init(struct core_lock *self);
int core_lock_lock(struct core_lock *self);
int core_lock_unlock(struct core_lock *self);
int core_lock_trylock(struct core_lock *self);
void core_lock_destroy(struct core_lock *self);
int core_lock_lock_private(int *lock);

#endif
