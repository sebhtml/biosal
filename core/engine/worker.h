
#ifndef BSAL_WORKER_H
#define BSAL_WORKER_H

#include <core/structures/fast_ring.h>
#include <core/structures/ring_queue.h>
#include <core/structures/set.h>

#include <core/system/memory_pool.h>
#include <core/system/thread.h>

#include <stdint.h>

struct bsal_work;
struct bsal_node;
struct bsal_message;

/*
#define BSAL_WORKER_USE_LOCK
*/

/*
 * Configuration of the buffering system of biosal
 */
#define BSAL_WORKER_RING_CAPACITY 512

/* Warning options for local work queue and
 * local message queue.
 */
#define BSAL_WORKER_WARNING_THRESHOLD 1
#define BSAL_WORKER_WARNING_THRESHOLD_STRIDE 128

/*
*/
#define BSAL_WORKER_HAS_OWN_QUEUES

#define BSAL_WORKER_USE_FAST_RINGS

/* this is similar to worker threads in linux ([kworker/0] [kworker/1])
 */
struct bsal_worker {
    struct bsal_node *node;

#ifdef BSAL_WORKER_USE_FAST_RINGS
    struct bsal_fast_ring work_queue;
    struct bsal_fast_ring message_queue;
#else
    struct bsal_ring work_queue;
    struct bsal_ring message_queue;
#endif

    struct bsal_ring_queue local_work_queue;
    struct bsal_ring_queue local_message_queue;

    struct bsal_thread thread;

#ifdef BSAL_WORKER_EVICTION
    struct bsal_lock eviction_lock;
    struct bsal_set actors_to_evict;
#endif

    int work_count;
    int start;

    int last_warning;

    int name;

    /* this is read by 2 threads, but written by 1 thread
     */
    int dead;

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

    struct bsal_memory_pool ephemeral_memory;
};

void bsal_worker_init(struct bsal_worker *worker, int name, struct bsal_node *node);
void bsal_worker_destroy(struct bsal_worker *worker);

void bsal_worker_start(struct bsal_worker *worker, int processor);
void bsal_worker_stop(struct bsal_worker *worker);

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

int bsal_worker_get_work_scheduling_score(struct bsal_worker *self);
int bsal_worker_get_message_production_score(struct bsal_worker *self);

int bsal_worker_pull_message(struct bsal_worker *worker, struct bsal_message *message);
int bsal_worker_push_work(struct bsal_worker *worker, struct bsal_work *work);

#endif

struct bsal_memory_pool *bsal_worker_get_ephemeral_memory(struct bsal_worker *worker);
void bsal_worker_queue_work(struct bsal_worker *worker, struct bsal_work *work);

int bsal_worker_dequeue_work(struct bsal_worker *worker, struct bsal_work *work);
int bsal_worker_enqueue_work(struct bsal_worker *worker, struct bsal_work *work);
void bsal_worker_evict_actors(struct bsal_worker *worker);
void bsal_worker_evict_actor(struct bsal_worker *worker, int actor_name);

#endif
