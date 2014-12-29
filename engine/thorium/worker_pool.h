
#ifndef THORIUM_WORKER_POOL_H
#define THORIUM_WORKER_POOL_H

#include "worker.h"

#include "scheduler/balancer.h"

#include <core/structures/fast_queue.h>
#include <core/structures/fast_ring.h>
#include <core/structures/vector.h>

#include <time.h>

struct thorium_node;
struct thorium_worker;
struct thorium_migration;
struct thorium_message;
struct thorium_message_block;

/*
#define THORIUM_WORKER_POOL_HAS_SPECIAL_QUEUES
*/

/*
#define THORIUM_WORKER_POOL_OUTBOUND_RING_SIZE 4
*/
#define THORIUM_WORKER_POOL_OUTBOUND_RING_SIZE 4095

/*
 * A worker pool.
 */
struct thorium_worker_pool {
    struct thorium_balancer balancer;
    struct core_timer timer;

#ifdef THORIUM_WORKER_USE_MULTIPLE_PRODUCER_RING
    struct core_fast_ring outbound_message_ring;
    struct core_fast_ring triage_message_ring;
#endif

    /*
    struct core_queue scheduled_actor_queue_buffer;
    */
    struct core_queue inbound_message_queue_buffer;
    struct core_queue clean_message_queue;

    int worker_count;
    struct core_vector worker_array;
    struct thorium_worker *worker_cache;
    char waiting_is_enabled;

#ifdef THORIUM_WORKER_POOL_USE_COUNT_CACHE
    struct core_vector message_count_cache;
    int *message_cache;
#endif

    struct thorium_node *node;

    int worker_for_message;
    int worker_for_run;
    int debug_mode;

    int last_warning;
    int last_scheduling_warning;

    time_t last_balancing;
    time_t last_signal_check;
    int balance_period;

    int ticks_without_messages;

    time_t starting_time;
    int worker_for_triage;

    int worker_for_demultiplex;

    uint64_t last_thorium_report_time;
};

#define THORIUM_WORKER_POOL_LOAD_LOOP 0
#define THORIUM_WORKER_POOL_LOAD_EPOCH 1

void thorium_worker_pool_init(struct thorium_worker_pool *self, int workers, struct thorium_node *node);
void thorium_worker_pool_destroy(struct thorium_worker_pool *self);

void thorium_worker_pool_create_workers(struct thorium_worker_pool *self);
void thorium_worker_pool_delete_workers(struct thorium_worker_pool *self);
void thorium_worker_pool_run(struct thorium_worker_pool *self);

void thorium_worker_pool_start(struct thorium_worker_pool *self);
void thorium_worker_pool_stop(struct thorium_worker_pool *self);

struct thorium_worker *thorium_worker_pool_select_worker_for_run(struct thorium_worker_pool *self);
struct thorium_worker *thorium_worker_pool_select_worker_for_message(struct thorium_worker_pool *self);
struct thorium_worker *thorium_worker_pool_select_worker_for_message_round_robin(struct thorium_worker_pool *pool);

int thorium_worker_pool_next_worker(struct thorium_worker_pool *node, int thread);

int thorium_worker_pool_worker_count(struct thorium_worker_pool *self);

struct thorium_worker *thorium_worker_pool_get_worker(
                struct thorium_worker_pool *self, int index);

void thorium_worker_pool_print_load(struct thorium_worker_pool *self, int type);

void thorium_worker_pool_toggle_debug_mode(struct thorium_worker_pool *self);

int thorium_worker_pool_enqueue_message(struct thorium_worker_pool *self, struct thorium_message *message);
int thorium_worker_pool_dequeue_message(struct thorium_worker_pool *self, struct thorium_message *message);

float thorium_worker_pool_get_computation_load(struct thorium_worker_pool *self);

struct thorium_node *thorium_worker_pool_get_node(struct thorium_worker_pool *self);

void thorium_worker_pool_work(struct thorium_worker_pool *self);
float thorium_worker_pool_get_current_load(struct thorium_worker_pool *self);

int thorium_worker_pool_dequeue_message_for_triage(struct thorium_worker_pool *self,
                struct thorium_message *message);

void thorium_worker_pool_examine(struct thorium_worker_pool *self);
void thorium_worker_pool_enable_profiler(struct thorium_worker_pool *self);

int thorium_worker_pool_buffered_message_count(struct thorium_worker_pool *self);
int thorium_worker_pool_outbound_ring_size(struct thorium_worker_pool *self);

void thorium_worker_pool_assign_worker_to_actor(struct thorium_worker_pool *self, int name);
int thorium_worker_pool_triage_message_queue_size(struct thorium_worker_pool *self);

#endif
