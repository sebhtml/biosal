
#ifndef THORIUM_WORKER_POOL_H
#define THORIUM_WORKER_POOL_H

#include "worker.h"

#include "scheduler/balancer.h"

#include <core/structures/fast_queue.h>
#include <core/structures/vector.h>

#include <time.h>

struct thorium_node;
struct thorium_worker;
struct thorium_migration;

/*
#define THORIUM_WORKER_POOL_HAS_SPECIAL_QUEUES
*/

/*
 * A worker pool.
 */
struct thorium_worker_pool {
    struct thorium_balancer scheduler;

    struct bsal_fast_queue scheduled_actor_queue_buffer;
    struct bsal_fast_queue inbound_message_queue_buffer;

    struct bsal_fast_queue messages_for_triage;

    int workers;
    struct bsal_vector worker_array;
    struct thorium_worker *worker_cache;
    char waiting_is_enabled;

    struct bsal_vector message_count_cache;
    int *message_cache;

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
int thorium_worker_pool_next_worker(struct thorium_worker_pool *node, int thread);

int thorium_worker_pool_worker_count(struct thorium_worker_pool *self);

struct thorium_worker *thorium_worker_pool_get_worker(
                struct thorium_worker_pool *self, int index);

void thorium_worker_pool_print_load(struct thorium_worker_pool *self, int type);

#ifdef THORIUM_WORKER_HAS_OWN_QUEUES
int thorium_worker_pool_pull_classic(struct thorium_worker_pool *self, struct thorium_message *message);
void thorium_worker_pool_schedule_work_classic(struct thorium_worker_pool *self, struct bsal_work *work);

#endif

void thorium_worker_pool_toggle_debug_mode(struct thorium_worker_pool *self);
void thorium_worker_pool_set_cached_value(struct thorium_worker_pool *self, int index, int value);
int thorium_worker_pool_get_cached_value(struct thorium_worker_pool *self, int index);

int thorium_worker_pool_enqueue_message(struct thorium_worker_pool *self, struct thorium_message *message);
int thorium_worker_pool_dequeue_message(struct thorium_worker_pool *self, struct thorium_message *message);

float thorium_worker_pool_get_computation_load(struct thorium_worker_pool *self);

struct thorium_node *thorium_worker_pool_get_node(struct thorium_worker_pool *self);
int thorium_worker_pool_give_message_to_actor(struct thorium_worker_pool *self, struct thorium_message *message);

void thorium_worker_pool_work(struct thorium_worker_pool *self);
void thorium_worker_pool_assign_worker_to_actor(struct thorium_worker_pool *self, int name);
float thorium_worker_pool_get_current_load(struct thorium_worker_pool *self);
void thorium_worker_pool_wake_up_workers(struct thorium_worker_pool *self);

int thorium_worker_pool_dequeue_message_for_triage(struct thorium_worker_pool *self,
                struct thorium_message *message);

void thorium_worker_pool_examine(struct thorium_worker_pool *self);
void thorium_worker_pool_enable_profiler(struct thorium_worker_pool *self);

#endif
