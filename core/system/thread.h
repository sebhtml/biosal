
#ifndef BIOSAL_THREAD_H
#define BIOSAL_THREAD_H

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
struct biosal_thread {
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

void biosal_thread_init(struct biosal_thread *self, void *(*function)(void *), void *argument);
void biosal_thread_destroy(struct biosal_thread *self);
void biosal_thread_set_affinity(struct biosal_thread *self, int processor);
void biosal_thread_start(struct biosal_thread *self);
void biosal_thread_join(struct biosal_thread *self);

void biosal_set_affinity(int processor);

void biosal_thread_wait(struct biosal_thread *self);
void biosal_thread_signal(struct biosal_thread *self);
uint64_t biosal_thread_get_wake_up_count(struct biosal_thread *self);

#endif
