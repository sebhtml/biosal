
#ifndef BSAL_THREAD_H
#define BSAL_THREAD_H

#include <pthread.h>

#include <stdint.h>

/*
 *
 * THis is a wrapper for a thread type.
 *
 * For the affinity:
 *
 * \see http://mechanical-sympathy.blogspot.com/2011/07/processor-affinity-part-1.html
 * \see http://mechanical-sympathy.blogspot.com/2013/02/cpu-cache-flushing-fallacy.html
 *
 * \see https://computing.llnl.gov/tutorials/pthreads/
 */
struct bsal_thread {
    pthread_t thread;
    pthread_attr_t attributes;
    int processor;
    void *(*function)(void *);
    void *argument;
    int affinity;

    char waiting;
    pthread_cond_t waiting_condition;
    pthread_mutex_t waiting_mutex;

    uint64_t wake_up_event_count;
};

void bsal_thread_init(struct bsal_thread *thread, void *(*function)(void *), void *argument);
void bsal_thread_destroy(struct bsal_thread *thread);
void bsal_thread_set_affinity(struct bsal_thread *thread, int processor);
void bsal_thread_start(struct bsal_thread *thread);
void bsal_thread_join(struct bsal_thread *thread);

void bsal_set_affinity(int processor);

void bsal_thread_wait(struct bsal_thread *thread);
void bsal_thread_signal(struct bsal_thread *thread);
uint64_t bsal_thread_get_wake_up_count(struct bsal_thread *thread);

#endif
