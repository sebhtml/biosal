
#ifndef BSAL_WORKER_H
#define BSAL_WORKER_H

#include "actor.h"

#include "scheduler/scheduling_queue.h"
#include "scheduler/priority_scheduler.h"

#include <core/structures/fast_ring.h>
#include <core/structures/ring_queue.h>
#include <core/structures/set.h>
#include <core/structures/map.h>
#include <core/structures/map_iterator.h>

#include <core/system/memory_pool.h>
#include <core/system/timer.h>
#include <core/system/thread.h>

#include <stdint.h>

struct bsal_work;
struct thorium_node;
struct thorium_message;
struct thorium_scheduler;

/*
 * Inject clean worker buffers into the worker rings
 */
#define BSAL_NODE_INJECT_CLEAN_WORKER_BUFFERS

/*
 * Enable wait and signal for workers
 */
/*
*/
#define BSAL_WORKER_ENABLE_WAIT

#define BSAL_WORKER_NONE (-99)

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
struct thorium_worker {
    struct thorium_node *node;

    struct bsal_timer timer;
    struct bsal_map actors;
    struct bsal_map_iterator actor_iterator;
    int ticks_without_production;

    char waiting_is_enabled;

    /*
     * The worker pool push actors to schedule on this
     * ring
     */
    struct bsal_fast_ring actors_to_schedule;

#ifdef BSAL_NODE_INJECT_CLEAN_WORKER_BUFFERS
    struct bsal_fast_ring injected_clean_outbound_buffers;

    struct bsal_fast_ring clean_message_ring_for_triage;
    struct bsal_ring_queue clean_message_queue_for_triage;
#endif

    struct thorium_scheduling_queue scheduling_queue;

    struct bsal_fast_ring outbound_message_queue;
    struct bsal_ring_queue outbound_message_queue_buffer;

    struct bsal_set evicted_actors;

    struct bsal_lock lock;

    struct bsal_thread thread;

    int work_count;
    char started_in_thread;

    int last_warning;

    int name;

    /* this is read by 2 threads, but written by 1 thread
     */
    int dead;

    int debug;

    /* this is read by 2 threads, but written by 1 thread
     */
    int busy;

    time_t last_report;
    uint64_t epoch_start_in_nanoseconds;
    uint64_t epoch_used_nanoseconds;
    float epoch_load;

    uint64_t loop_start_in_nanoseconds;
    uint64_t loop_used_nanoseconds;
    uint64_t loop_end_in_nanoseconds;

    uint64_t scheduling_epoch_start_in_nanoseconds;
    uint64_t scheduling_epoch_used_nanoseconds;

    struct bsal_memory_pool ephemeral_memory;
    struct bsal_memory_pool outbound_message_memory_pool;

    struct thorium_priority_scheduler scheduler;

    uint64_t last_wake_up_count;

    uint64_t waiting_start_time;
};

void thorium_worker_init(struct thorium_worker *self, int name, struct thorium_node *node);
void thorium_worker_destroy(struct thorium_worker *self);

void thorium_worker_start(struct thorium_worker *self, int processor);
void thorium_worker_stop(struct thorium_worker *self);

void thorium_worker_run(struct thorium_worker *self);
void thorium_worker_work(struct thorium_worker *self, struct thorium_actor *actor);
struct thorium_node *thorium_worker_node(struct thorium_worker *self);

void thorium_worker_send(struct thorium_worker *self, struct thorium_message *message);

void *thorium_worker_main(void *worker1);
int thorium_worker_name(struct thorium_worker *self);
void thorium_worker_display(struct thorium_worker *self);

int thorium_worker_is_busy(struct thorium_worker *self);

float thorium_worker_get_epoch_load(struct thorium_worker *self);
float thorium_worker_get_loop_load(struct thorium_worker *self);
float thorium_worker_get_scheduling_epoch_load(struct thorium_worker *self);
void thorium_worker_reset_scheduling_epoch(struct thorium_worker *self);

int thorium_worker_get_scheduled_message_count(struct thorium_worker *self);
int thorium_worker_get_message_production_score(struct thorium_worker *self);

struct bsal_memory_pool *thorium_worker_get_ephemeral_memory(struct thorium_worker *self);

int thorium_worker_dequeue_actor(struct thorium_worker *self, struct thorium_actor **actor);
int thorium_worker_enqueue_actor(struct thorium_worker *self, struct thorium_actor *actor);
int thorium_worker_enqueue_actor_special(struct thorium_worker *self, struct thorium_actor *actor);

int thorium_worker_enqueue_message(struct thorium_worker *self, struct thorium_message *message);
int thorium_worker_dequeue_message(struct thorium_worker *self, struct thorium_message *message);

void thorium_worker_print_actors(struct thorium_worker *self, struct thorium_scheduler *scheduler);

void thorium_worker_evict_actor(struct thorium_worker *self, int actor_name);
void thorium_worker_lock(struct thorium_worker *self);
void thorium_worker_unlock(struct thorium_worker *self);
struct bsal_map *thorium_worker_get_actors(struct thorium_worker *self);

int thorium_worker_get_sum_of_received_actor_messages(struct thorium_worker *self);
int thorium_worker_get_queued_messages(struct thorium_worker *self);
int thorium_worker_get_production(struct thorium_worker *self, struct thorium_scheduler *scheduler);
int thorium_worker_get_producer_count(struct thorium_worker *self, struct thorium_scheduler *scheduler);

void thorium_worker_free_message(struct thorium_worker *self, struct thorium_message *message);

void thorium_worker_wait(struct thorium_worker *self);
void thorium_worker_signal(struct thorium_worker *self);

uint64_t thorium_worker_get_epoch_wake_up_count(struct thorium_worker *self);
uint64_t thorium_worker_get_loop_wake_up_count(struct thorium_worker *self);

void thorium_worker_enable_waiting(struct thorium_worker *self);
time_t thorium_worker_get_last_report_time(struct thorium_worker *self);
void thorium_worker_check_production(struct thorium_worker *self, int value, int name);

/*
 * Inject a clean buffer into the worker
 */
int thorium_worker_inject_clean_outbound_buffer(struct thorium_worker *self, void *buffer);
int thorium_worker_fetch_clean_outbound_buffer(struct thorium_worker *self, void **buffer);
int thorium_worker_enqueue_message_for_triage(struct thorium_worker *worker, struct thorium_message *message);
int thorium_worker_dequeue_message_for_triage(struct thorium_worker *worker, struct thorium_message *message);

#endif
