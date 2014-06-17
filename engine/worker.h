
#ifndef BSAL_THREAD_H
#define BSAL_THREAD_H

#include <structures/queue.h>

#include <system/lock.h>

struct bsal_work;
struct bsal_node;
struct bsal_message;

#define BSAL_WORKER_USE_LOCK

/* this is similar to worker threads in linux ([kworker/0] [kworker/1])
 */
struct bsal_worker {
    struct bsal_node *node;
    pthread_t thread;

    int name;

    /* this is read by 2 threads, but written by 1 thread
     */
    volatile int dead;

    struct bsal_queue works;
    struct bsal_queue messages;

#ifdef BSAL_WORKER_USE_LOCK
    struct bsal_lock work_lock;
    struct bsal_lock message_lock;
#endif

    int debug;

    /* this is read by 2 threads, but written by 1 thread
     */
    volatile int busy;
};

void bsal_worker_init(struct bsal_worker *worker, int name, struct bsal_node *node);
void bsal_worker_destroy(struct bsal_worker *worker);

struct bsal_queue *bsal_worker_works(struct bsal_worker *worker);
struct bsal_queue *bsal_worker_messages(struct bsal_worker *worker);

void bsal_worker_start(struct bsal_worker *worker);
void bsal_worker_stop(struct bsal_worker *worker);
pthread_t *bsal_worker_thread(struct bsal_worker *worker);

void bsal_worker_run(struct bsal_worker *worker);
void bsal_worker_work(struct bsal_worker *worker, struct bsal_work *work);
struct bsal_node *bsal_worker_node(struct bsal_worker *worker);

void bsal_worker_send(struct bsal_worker *worker, struct bsal_message *message);

void *bsal_worker_main(void *worker1);
int bsal_worker_name(struct bsal_worker *worker);
void bsal_worker_display(struct bsal_worker *worker);

void bsal_worker_push_work(struct bsal_worker *worker, struct bsal_work *work);
int bsal_worker_pull_work(struct bsal_worker *worker, struct bsal_work *work);

void bsal_worker_push_message(struct bsal_worker *worker, struct bsal_message *message);
int bsal_worker_pull_message(struct bsal_worker *worker, struct bsal_message *message);

int bsal_worker_is_busy(struct bsal_worker *self);
int bsal_worker_enqueued_work_count(struct bsal_worker *self);

#endif
