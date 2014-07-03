
#define _GNU_SOURCE

#include "thread.h"

#if defined(__linux__) || defined(__bgq__)

#include <sched.h>
#endif

#include <stdio.h>

/* for getpid */
#include <unistd.h>

void bsal_thread_init(struct bsal_thread *self, void *(*function)(void *), void *argument)
{
    self->function = function;
    self->argument = argument;
    self->processor = -1;
    self->affinity = 0;

    pthread_attr_init(&self->attributes);
}

void bsal_thread_destroy(struct bsal_thread *self)
{
    self->function = NULL;
    self->argument = NULL;
    self->processor = -1;
}

void bsal_thread_start(struct bsal_thread *self)
{
#if defined(__linux__) || defined(__bgq__)
    cpu_set_t mask;

    if (self->processor >= 0) {
        CPU_ZERO(&mask);
        CPU_SET(self->processor, &mask);

        /*
         * \see http://man7.org/linux/man-pages/man3/pthread_attr_setaffinity_np.3.html
         * \see http://pubs.opengroup.org/onlinepubs/009695399/functions/pthread_self.html
         * \see http://stackoverflow.com/questions/1407786/how-to-set-cpu-affinity-of-a-particular-pthread
         * \see http://newsgroups.derkeiler.com/Archive/Comp/comp.programming.threads/2006-06/msg00269.html
         */
#if defined(__linux__) || defined(__GNUC__)
        if (pthread_attr_setaffinity_np(&self->attributes, sizeof(mask), &mask) != 0) {
            printf("Warning: could not set affinity.\n");
        } else {
            self->affinity = 1;
        }

        /*
         * See http://newsgroups.derkeiler.com/Archive/Comp/comp.programming.threads/2006-06/msg00269.html
         */
#elif defined(__bgq__)
        if (pthread_attr_setaffinity(&self->attributes, sizeof(mask), &mask) == -1) {
            printf("Warning: could not set affinity.\n");
        } else {
            self->affinity = 1;

        }
#endif
    }
#endif

    /*
     * http://pubs.opengroup.org/onlinepubs/7908799/xsh/pthread_create.html
     */
    pthread_create(&self->thread, &self->attributes, self->function, self->argument);

    if (self->affinity) {

        printf("AFFINITY: affinity of thread %p has been set to CPU %d\n",
                            (void *)&self->thread, self->processor);
    }
}

void bsal_thread_set_affinity(struct bsal_thread *self, int processor)
{
    self->processor = processor;
}

void bsal_thread_join(struct bsal_thread *self)
{
    /* http://man7.org/linux/man-pages/man3/pthread_join.3.html
     */
    pthread_join(self->thread, NULL);
}

void bsal_set_affinity(int processor)
{
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
