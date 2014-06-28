
#ifndef BSAL_WORKER_H
#define BSAL_WORKER_H

#include "message_queue.h"
#include <structures/ring.h>
#include <structures/ring_queue.h>

#include <stdint.h>

struct bsal_work;
struct bsal_node;
struct bsal_message;

#define BSAL_WORKER_USE_LOCK

/*
*/
#define BSAL_WORKER_HAS_OWN_QUEUES

/* this is similar to worker threads in linux ([kworker/0] [kworker/1])
 */
struct bsal_worker {
    struct bsal_node *node;

    struct bsal_ring work_queue;
    struct bsal_ring message_queue;

    struct bsal_ring_queue local_message_queue;
    pthread_t thread;

    int work_count;
    int start;

    int name;

    /* this is read by 2 threads, but written by 1 thread
     */
    int dead;

#ifdef BSAL_WORKER_HAS_OWN_QUEUES
    /*struct bsal_work_queue works;*/
    struct bsal_message_queue messages;

#endif

    int debug;

    /* this is read by 2 threads, but written by 1 thread
     */
    int busy;

    uint64_t last_report;
    uint64_t epoch_start_in_nanoseconds;
    uint64_t epoch_used_nanoseconds;
    float epoch_load;

    uint64_t loop_start_in_nanoseconds;
    uint64_t loop_used_nanoseconds;
    float loop_load;
};

void bsal_worker_init(struct bsal_worker *worker, int name, struct bsal_node *node);
void bsal_worker_destroy(struct bsal_worker *worker);

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

int bsal_worker_pull_work(struct bsal_worker *worker, struct bsal_work *work);

void bsal_worker_push_message(struct bsal_worker *worker, struct bsal_message *message);

int bsal_worker_is_busy(struct bsal_worker *self);

float bsal_worker_get_epoch_load(struct bsal_worker *self);
float bsal_worker_get_loop_load(struct bsal_worker *self);

#ifdef BSAL_WORKER_HAS_OWN_QUEUES

int bsal_worker_enqueued_work_count(struct bsal_worker *self);
int bsal_worker_enqueued_message_count(struct bsal_worker *self);

int bsal_worker_get_work_scheduling_score(struct bsal_worker *self);
int bsal_worker_get_message_production_score(struct bsal_worker *self);

int bsal_worker_pull_message(struct bsal_worker *worker, struct bsal_message *message);
int bsal_worker_push_work(struct bsal_worker *worker, struct bsal_work *work);

#endif

#endif
