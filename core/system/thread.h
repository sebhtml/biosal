
#ifndef CORE_THREAD_H
#define CORE_THREAD_H

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
struct core_thread {
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

void core_thread_init(struct core_thread *self, void *(*function)(void *), void *argument);
void core_thread_destroy(struct core_thread *self);
void core_thread_set_affinity(struct core_thread *self, int processor);
void core_thread_start(struct core_thread *self);
void core_thread_join(struct core_thread *self);

void core_set_affinity(int processor);

void core_thread_wait(struct core_thread *self);
void core_thread_signal(struct core_thread *self);
uint64_t core_thread_get_wake_up_count(struct core_thread *self);

#endif
