
#ifndef _BSAL_THREAD_H
#define _BSAL_THREAD_H

#include <pthread.h>

/* For the affinity:
 * \see http://mechanical-sympathy.blogspot.com/2011/07/processor-affinity-part-1.html
 * \see http://mechanical-sympathy.blogspot.com/2013/02/cpu-cache-flushing-fallacy.html
 */
struct bsal_thread {
    pthread_t thread;
    pthread_attr_t attributes;
    int processor;
    void *(*function)(void *);
    void *argument;
    int affinity;
};

void bsal_thread_init(struct bsal_thread *self, void *(*function)(void *), void *argument);
void bsal_thread_destroy(struct bsal_thread *self);
void bsal_thread_set_affinity(struct bsal_thread *self, int processor);
void bsal_thread_start(struct bsal_thread *self);
void bsal_thread_join(struct bsal_thread *self);

void bsal_set_affinity(int processor);

#endif
