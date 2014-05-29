
#ifndef _BSAL_THREAD_H
#define _BSAL_THREAD_H

#include <structures/fifo.h>

#include <pthread.h>

struct bsal_work;
struct bsal_node;
struct bsal_message;

#define BSAL_THREAD_USE_LOCK

/* this is similar to worker threads in linux ([kworker/0] [kworker/1])
 */
struct bsal_worker_thread {
    struct bsal_node *node;
    pthread_t thread;

    int name;
    volatile int dead;

    struct bsal_fifo works;
    struct bsal_fifo messages;

#ifdef BSAL_THREAD_USE_LOCK
    pthread_spinlock_t work_lock;
    pthread_spinlock_t message_lock;
#endif
};

void bsal_worker_thread_init(struct bsal_worker_thread *thread, int name, struct bsal_node *node);
void bsal_worker_thread_destroy(struct bsal_worker_thread *thread);

struct bsal_fifo *bsal_worker_thread_works(struct bsal_worker_thread *thread);
struct bsal_fifo *bsal_worker_thread_messages(struct bsal_worker_thread *thread);

void bsal_worker_thread_start(struct bsal_worker_thread *thread);
void bsal_worker_thread_stop(struct bsal_worker_thread *thread);
pthread_t *bsal_worker_thread_thread(struct bsal_worker_thread *thread);

void bsal_worker_thread_run(struct bsal_worker_thread *thread);
void bsal_worker_thread_work(struct bsal_worker_thread *thread, struct bsal_work *work);
struct bsal_node *bsal_worker_thread_node(struct bsal_worker_thread *thread);

void bsal_worker_thread_send(struct bsal_worker_thread *thread, struct bsal_message *message);

void *bsal_worker_thread_main(void *pointer);
int bsal_worker_thread_name(struct bsal_worker_thread *thread);
void bsal_worker_thread_display(struct bsal_worker_thread *thread);

void bsal_worker_thread_push_work(struct bsal_worker_thread *thread, struct bsal_work *work);
int bsal_worker_thread_pull_work(struct bsal_worker_thread *thread, struct bsal_work *work);

void bsal_worker_thread_push_message(struct bsal_worker_thread *thread, struct bsal_message *message);
int bsal_worker_thread_pull_message(struct bsal_worker_thread *thread, struct bsal_message *message);

#endif
