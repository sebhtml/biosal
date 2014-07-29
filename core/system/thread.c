
/*
 * This must be defined before any include
 * in the C file
 */
#define _GNU_SOURCE

#include "thread.h"

#include "memory.h"


#if defined(__linux__) || defined(__bgq__)

#include <sched.h>

#endif

#include <stdio.h>

/* for getpid */
#include <unistd.h>

/*
#define BSAL_THREAD_DEBUG_WAIT
*/
/*
 * Enable thread affinity by uncommenting this option.
 */
/*
#define BSAL_THREAD_SET_AFFINITY
*/
void bsal_thread_init(struct bsal_thread *thread, void *(*function)(void *), void *argument)
{
    thread->function = function;
    thread->argument = argument;

    /* A negative processor disables the affinity code.
     */
    thread->processor = -1;

    thread->affinity = 0;

    pthread_attr_init(&thread->attributes);

    pthread_mutex_init(&thread->waiting_mutex, NULL);
    pthread_cond_init(&thread->waiting_condition, NULL);

    thread->waiting = 0;
    thread->wake_up_event_count= 0;
}

void bsal_thread_destroy(struct bsal_thread *thread)
{
    thread->function = NULL;
    thread->argument = NULL;
    thread->processor = -1;

    pthread_mutex_destroy(&thread->waiting_mutex);
    pthread_cond_destroy(&thread->waiting_condition);
}

void bsal_thread_start(struct bsal_thread *thread)
{
    int set_affinity;

    set_affinity = 0;

#ifdef BSAL_THREAD_SET_AFFINITY
    set_affinity = 1;
#endif

#if defined(__linux__) || defined(__bgq__)
    cpu_set_t mask;

    if (set_affinity && thread->processor >= 0) {
        CPU_ZERO(&mask);
        CPU_SET(thread->processor, &mask);

        /*
         * \see http://man7.org/linux/man-pages/man3/pthread_attr_setaffinity_np.3.html
         * \see http://pubs.opengroup.org/onlinepubs/009695399/functions/pthread_thread.html
         * \see http://stackoverflow.com/questions/1407786/how-to-set-cpu-affinity-of-a-particular-pthread
         * \see http://newsgroups.derkeiler.com/Archive/Comp/comp.programming.threads/2006-06/msg00269.html
         */
#if defined(__linux__) || defined(__GNUC__)
        if (pthread_attr_setaffinity_np(&thread->attributes, sizeof(mask), &mask) != 0) {
            printf("Warning: could not set affinity.\n");
        } else {
            thread->affinity = 1;
        }

        /*
         * See http://newsgroups.derkeiler.com/Archive/Comp/comp.programming.threads/2006-06/msg00269.html
         */
#elif defined(__bgq__)
        if (pthread_attr_setaffinity(&thread->attributes, sizeof(mask), &mask) == -1) {
            printf("Warning: could not set affinity.\n");
        } else {
            thread->affinity = 1;

        }
#endif
    }
#endif

    /*
     * http://pubs.opengroup.org/onlinepubs/7908799/xsh/pthread_create.html
     */
    pthread_create(&thread->thread, &thread->attributes, thread->function, thread->argument);

    if (thread->affinity) {

        printf("AFFINITY: affinity of thread %p has been set to CPU %d\n",
                            (void *)&thread->thread, thread->processor);
    }
}

void bsal_thread_set_affinity(struct bsal_thread *thread, int processor)
{
    /*
     * Keep the processor value to -1
     * if affinity is not enabled.
     */
#ifdef BSAL_THREAD_SET_AFFINITY
    thread->processor = processor;
#endif
}

void bsal_thread_join(struct bsal_thread *thread)
{
    /* http://man7.org/linux/man-pages/man3/pthread_join.3.html
     */
    pthread_join(thread->thread, NULL);
}

void bsal_set_affinity(int processor)
{
    int set_affinity;

    set_affinity = 0;

#ifdef BSAL_THREAD_SET_AFFINITY
    set_affinity = 1;
#endif

    /* Don't set affinity if it is disabled...
     */
    if (!set_affinity) {
        return;
    }

#if defined(__linux__) || defined(__bgq__)
    cpu_set_t mask;
    pid_t process;

    if (processor < 0) {
        return;
    }

    process = getpid();
    CPU_ZERO(&mask);
    CPU_SET(processor, &mask);

    sched_setaffinity(process, sizeof(mask), &mask);

    printf("AFFINITY: the affinity of process %d has been set to CPU %d\n",
                    process, processor);
#endif
}

/*
 * If I run thorium with 16 threads, it uses 1 for pacing and 15 worker threads.
 *
 * If I run thorium with 32 threads, it uses 1 for pacing and 31 worker threads.
 *
 * In the current code, workers do busy polling to dequeue actors from the scheduling queue
 * (each worker has one of these). But this is useless when no actors are in the scheduling queue .
 *
 * I will add notify/wait so that a worker wait (pthread_cond_wait) when it has nothing in its
 * scheduling queue and notify (pthread_cond_signal) to resume the thread.
 *
 * I think this will make the code go faster for I/O since at the moment thorium workers
 * are mostly just busy polling during I/O.
 */
/*
 * Based on the pseudocode at https://computing.llnl.gov/tutorials/pthreads/
 */
void bsal_thread_wait(struct bsal_thread *thread)
{
    if (thread->waiting) {
        /* Don't do anything since the thread is already waiting.
         */
        return;
    }

#ifdef BSAL_THREAD_DEBUG_WAIT
    printf("DEBUG bsal_thread_wait: Getting lock now\n");
#endif

    pthread_mutex_lock(&thread->waiting_mutex);

    /*
     * Make the waiting variable visible.
     */
    thread->waiting = 1;
    ++thread->wake_up_event_count;

    bsal_memory_fence();

#ifdef BSAL_THREAD_DEBUG_WAIT
    printf("DEBUG bsal_thread_wait enter pthread_cond_wait\n");
#endif
    /*
     * The wait call below unlocks the mutex such that
     * others can access it.
     * The call will return when the thread receives the POSIX
     * thread signal.
     */
    pthread_cond_wait(&thread->waiting_condition, &thread->waiting_mutex);

#ifdef BSAL_THREAD_DEBUG_WAIT
    printf("DEBUG bsal_thread_wait exit pthread_cond_wait\n");
#endif
    /*
     * OK. The thread is not waiting anymore. The thread owns the waiting mutex
     * though now, so it needs to be unlocked somehow.
     *
     * Set the waiting variable to 0 and make it visible
     */
    thread->waiting = 0;
    bsal_memory_fence();

    /*
     * Here, the thread gets the control, and the waiting_mutex is now locked.
     * it needs to be unlocked now.
     */
    pthread_mutex_unlock(&thread->waiting_mutex);

#ifdef BSAL_THREAD_DEBUG_WAIT
    printf("DEBUG bsal_thread_wait released lock\n");
#endif
}

void bsal_thread_signal(struct bsal_thread *thread)
{
    /* Don't signal if the thread is not waiting.
     * A lock is not required since the wait call
     * is using a memory fence.
     */
    if (!thread->waiting) {
        return;
    }

#ifdef BSAL_THREAD_DEBUG_WAIT
    printf("DEBUG bsal_thread_signal sending signal to thread.\n");
#endif

    /* Otherwise, signal the thread.
     */
    pthread_mutex_lock(&thread->waiting_mutex);

#ifdef BSAL_THREAD_DEBUG_WAIT
    printf("DEBUG bsal_thread_signal signal got lock\n");
#endif

    pthread_cond_signal(&thread->waiting_condition);
    pthread_mutex_unlock(&thread->waiting_mutex);
}

uint64_t bsal_thread_get_wake_up_count(struct bsal_thread *thread)
{
    return thread->wake_up_event_count;
}
