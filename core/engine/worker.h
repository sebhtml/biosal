
#ifndef BSAL_WORKER_H
#define BSAL_WORKER_H

#include "actor.h"

#include <core/structures/fast_ring.h>
#include <core/structures/ring_queue.h>
#include <core/structures/set.h>

#include <core/system/memory_pool.h>
#include <core/system/thread.h>

#include <stdint.h>

struct bsal_work;
struct bsal_node;
struct bsal_message;
struct bsal_scheduler;

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

    struct bsal_map actors;

    struct bsal_fast_ring scheduled_actor_queue;
    struct bsal_ring_queue scheduled_actor_queue_real;

    struct bsal_fast_ring outbound_message_queue;
    struct bsal_ring_queue outbound_message_queue_buffer;

    struct bsal_set evicted_actors;

    struct bsal_lock lock;

    struct bsal_thread thread;

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

    uint64_t scheduling_epoch_start_in_nanoseconds;
    uint64_t scheduling_epoch_used_nanoseconds;
    float loop_load;

    struct bsal_memory_pool ephemeral_memory;
};

void bsal_worker_init(struct bsal_worker *worker, int name, struct bsal_node *node);
void bsal_worker_destroy(struct bsal_worker *worker);

void bsal_worker_start(struct bsal_worker *worker, int processor);
void bsal_worker_stop(struct bsal_worker *worker);

void bsal_worker_run(struct bsal_worker *worker);
void bsal_worker_work(struct bsal_worker *worker, struct bsal_actor *actor);
struct bsal_node *bsal_worker_node(struct bsal_worker *worker);

void bsal_worker_send(struct bsal_worker *worker, struct bsal_message *message);

void *bsal_worker_main(void *worker1);
int bsal_worker_name(struct bsal_worker *worker);
void bsal_worker_display(struct bsal_worker *worker);

int bsal_worker_is_busy(struct bsal_worker *self);

float bsal_worker_get_epoch_load(struct bsal_worker *self);
float bsal_worker_get_loop_load(struct bsal_worker *self);
float bsal_worker_get_scheduling_epoch_load(struct bsal_worker *worker);
void bsal_worker_reset_scheduling_epoch(struct bsal_worker *worker);

int bsal_worker_get_scheduled_message_count(struct bsal_worker *self);
int bsal_worker_get_message_production_score(struct bsal_worker *self);

struct bsal_memory_pool *bsal_worker_get_ephemeral_memory(struct bsal_worker *worker);

int bsal_worker_dequeue_actor(struct bsal_worker *worker, struct bsal_actor **actor);
int bsal_worker_enqueue_actor(struct bsal_worker *worker, struct bsal_actor **actor);
int bsal_worker_enqueue_actor_special(struct bsal_worker *worker, struct bsal_actor **actor);

int bsal_worker_enqueue_message(struct bsal_worker *worker, struct bsal_message *message);
int bsal_worker_dequeue_message(struct bsal_worker *worker, struct bsal_message *message);

void bsal_worker_print_actors(struct bsal_worker *worker, struct bsal_scheduler *scheduler);

void bsal_worker_evict_actor(struct bsal_worker *worker, int actor_name);
void bsal_worker_lock(struct bsal_worker *worker);
void bsal_worker_unlock(struct bsal_worker *worker);
struct bsal_map *bsal_worker_get_actors(struct bsal_worker *worker);

int bsal_worker_get_sum_of_received_actor_messages(struct bsal_worker *self);
int bsal_worker_get_queued_messages(struct bsal_worker *self);
int bsal_worker_get_production(struct bsal_worker *worker, struct bsal_scheduler *scheduler);
int bsal_worker_get_producer_count(struct bsal_worker *worker, struct bsal_scheduler *scheduler);

#endif
