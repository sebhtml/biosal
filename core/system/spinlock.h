
#ifndef CORE_SPINLOCK_H
#define CORE_SPINLOCK_H

#include <pthread.h>

#include "atomic.h"

#define CORE_SPINLOCK_SUCCESS 0
#define CORE_SPINLOCK_ERROR -1

#define CORE_SPINLOCK_UNLOCKED 0
#define CORE_SPINLOCK_LOCKED 1

#if defined(__linux__) && defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L
#define CORE_SPINLOCK_USE_SPIN_LOCK

#elif defined(__bgq__)
#define CORE_SPINLOCK_USE_SPIN_LOCK

#elif defined(__APPLE__) || defined(MACOSX) || defined(__unix__)
#define CORE_SPINLOCK_USE_MUTEX

#else
#define CORE_SPINLOCK_USE_MUTEX

#endif

/*
Uncomment this to force mutexes
#define CORE_SPINLOCK_USE_MUTEX
*/

/* Remove spinlock define if CORE_SPINLOCK_USE_MUTEX is defined
 * by the user
 */
#ifdef CORE_SPINLOCK_USE_MUTEX
#ifdef CORE_SPINLOCK_USE_SPIN_LOCK
#undef CORE_SPINLOCK_USE_SPIN_LOCK
#endif
#endif

/*
 * Uncomment this to use the custom spinlock (which is
 * not very good.
 *
 */
/*
#ifdef CORE_ATOMIC_HAS_COMPARE_AND_SWAP
#define CORE_SPINLOCK_USE_COMPARE_AND_SWAP
#endif
*/

/*
 * A lock.
 */
struct core_spinlock {

#if defined(CORE_SPINLOCK_USE_COMPARE_AND_SWAP)
    int lock;

#elif defined(CORE_SPINLOCK_USE_SPIN_LOCK)
/*#error "SPIN LOCK"*/
    pthread_spinlock_t lock;
#elif defined(CORE_SPINLOCK_USE_MUTEX)
    pthread_mutex_t lock;
#endif

};

void core_spinlock_init(struct core_spinlock *self);
void core_spinlock_destroy(struct core_spinlock *self);

int core_spinlock_lock(struct core_spinlock *self);
int core_spinlock_unlock(struct core_spinlock *self);
int core_spinlock_trylock(struct core_spinlock *self);

#endif
