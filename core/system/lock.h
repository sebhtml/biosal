
#ifndef BIOSAL_LOCK_H
#define BIOSAL_LOCK_H

#include <pthread.h>

#include "atomic.h"

#define BIOSAL_LOCK_SUCCESS 0
#define BIOSAL_LOCK_ERROR -1

#define BIOSAL_LOCK_UNLOCKED 0
#define BIOSAL_LOCK_LOCKED 1

#if defined(__linux__) && defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L
#define BIOSAL_LOCK_USE_SPIN_LOCK

#elif defined(__bgq__)
#define BIOSAL_LOCK_USE_SPIN_LOCK

#elif defined(__APPLE__) || defined(MACOSX) || defined(__unix__)
#define BIOSAL_LOCK_USE_MUTEX

#else
#define BIOSAL_LOCK_USE_MUTEX

#endif

/*
Uncomment this to force mutexes
#define BIOSAL_LOCK_USE_MUTEX
*/

/* Remove spinlock define if BIOSAL_LOCK_USE_MUTEX is defined
 * by the user
 */
#ifdef BIOSAL_LOCK_USE_MUTEX
#ifdef BIOSAL_LOCK_USE_SPIN_LOCK
#undef BIOSAL_LOCK_USE_SPIN_LOCK
#endif
#endif

/*
 * Uncomment this to use the custom spinlock (which is
 * not very good.
 *
 */
/*
#ifdef BIOSAL_ATOMIC_HAS_COMPARE_AND_SWAP
#define BIOSAL_LOCK_USE_COMPARE_AND_SWAP
#endif
*/

struct biosal_lock {

#if defined(BIOSAL_LOCK_USE_COMPARE_AND_SWAP)
    int lock;

#elif defined(BIOSAL_LOCK_USE_SPIN_LOCK)
/*#error "SPIN LOCK"*/
    pthread_spinlock_t lock;
#elif defined(BIOSAL_LOCK_USE_MUTEX)
    pthread_mutex_t lock;
#endif

};

void biosal_lock_init(struct biosal_lock *self);
int biosal_lock_lock(struct biosal_lock *self);
int biosal_lock_unlock(struct biosal_lock *self);
int biosal_lock_trylock(struct biosal_lock *self);
void biosal_lock_destroy(struct biosal_lock *self);
int biosal_lock_lock_private(int *lock);

#endif
