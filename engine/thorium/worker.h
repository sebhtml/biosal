
#ifndef THORIUM_WORKER_H
#define THORIUM_WORKER_H

#include "actor.h"

#include "message.h"

/*
#include "message_block.h"
*/

#include "actor_profiler.h"

#include "scheduler/scheduler.h"
#include "scheduler/priority_assigner.h"

#include "transport/message_multiplexer.h"
#include "transport/multiplexer_policy.h"

#include <core/structures/fast_ring.h>
#include <core/structures/fast_queue.h>
#include <core/structures/set.h>
#include <core/structures/map.h>
#include <core/structures/map_iterator.h>

#include <core/file_storage/output/buffered_file_writer.h>

#include <core/system/memory_pool.h>
#include <core/system/timer.h>
#include <core/system/thread.h>
#include <core/system/debugger.h>

#include <stdint.h>

/*
 * Use a multiple-producer lockless ring for outbound messages.
 */
#define THORIUM_WORKER_USE_MULTIPLE_PRODUCER_RING

struct biosal_work;
struct thorium_node;
struct thorium_message;
struct thorium_balancer;

/*
 * Enable locks in workers.
 */
/*
#define THORIUM_WORKER_ENABLE_LOCK
*/

/*
 * Inject clean worker buffers into the worker rings
 */
#define THORIUM_NODE_INJECT_CLEAN_WORKER_BUFFERS

/*
 * Enable wait and signal for workers
 */
/*
#define THORIUM_WORKER_ENABLE_WAIT
*/

#define THORIUM_WORKER_NONE (-99)

/*
#define THORIUM_WORKER_USE_LOCK
*/

/*
 * Configuration of the buffering system of biosal
 */
#define THORIUM_WORKER_RING_CAPACITY 512

/* Warning options for local work queue and
 * local message queue.
 */
#define THORIUM_WORKER_WARNING_THRESHOLD 1
#define THORIUM_WORKER_WARNING_THRESHOLD_STRIDE 128

/*
 * Some fancy compilation options.
 */
#define THORIUM_WORKER_HAS_OWN_QUEUES
#define THORIUM_WORKER_USE_FAST_RINGS

/*
 * Ring backoff for increased efficiency.
 *
 * \see http://en.wiktionary.org/wiki/back_off
 */
#define THORIUM_WORKER_CONFIG_USE_BACKOFF

/*
 * A Thorium worker.
 *
 * It has 4 rings:
 *
 * 1. input_inbound_message_ring
 * 2. input_clean_outbound_buffer_ring
 * 3. output_outbound_message_ring
 * 4. output_message_queue_for_triage
 *
 * This is similar to worker threads in linux ([kworker/0] [kworker/1])
 */
struct thorium_worker {
    struct core_buffered_file_writer load_profile_writer;
    struct thorium_node *node;

    struct thorium_worker *workers;
    int worker_count;

    /*
    struct thorium_message_block message_block;
    */

    struct core_timer timer;
    struct core_map actors;
    struct core_map_iterator actor_iterator;
    int ticks_without_production;

    /*
     * Buffer for zero-copy send.
     */
    void *zero_copy_buffer;

    int tick_count;
    uint64_t last_elapsed_nanoseconds;

    /*
     * The worker pool push actors to schedule on this
     * ring.
     */
    struct core_fast_ring input_inbound_message_ring;

    struct core_fast_ring input_message_ring_for_multiplexer;
    struct core_fast_queue output_outbound_message_queue_for_multiplexer;
    struct thorium_message_multiplexer multiplexer;
    struct thorium_multiplexer_policy multiplexer_policy;

    struct core_fast_queue input_inbound_message_queue;

#ifdef THORIUM_NODE_INJECT_CLEAN_WORKER_BUFFERS
    /*
     * Injected buffers for recycling.
     */
    struct core_fast_ring input_clean_outbound_buffer_ring;

    /*
     * Output ring for exporting messages for triage.
     */
    struct core_fast_ring *output_message_ring_for_triage;

    /*
     * Queue for buffering exported messages for triage.
     */
    struct core_fast_queue output_message_queue_for_triage;
#endif

    /*
     * Ring for publishing outbound buffers.
     */
#ifdef THORIUM_WORKER_USE_MULTIPLE_PRODUCER_RING
    struct core_fast_ring *output_outbound_message_ring_multiple;
#else
    struct core_fast_ring output_outbound_message_ring;
#endif

    /*
     * Queue for buffering outbound buffers when
     * the ring is full.
     */
    struct core_fast_queue output_outbound_message_queue;

    struct thorium_scheduler scheduler;

    struct core_set evicted_actors;

#ifdef THORIUM_WORKER_ENABLE_LOCK
    struct core_lock lock;
#endif

    struct core_thread thread;

    int work_count;
    char started_in_thread;

    int last_warning;

    int name;

    /* this is read by 2 threads, but written by 1 thread
     */
    uint32_t flags;

    struct core_map actor_received_messages;

    time_t last_report;
    uint64_t epoch_start_in_nanoseconds;
    uint64_t epoch_used_nanoseconds;
    float epoch_load;

    uint64_t loop_start_in_nanoseconds;
    uint64_t loop_used_nanoseconds;
    uint64_t loop_end_in_nanoseconds;

    uint64_t scheduling_epoch_start_in_nanoseconds;
    uint64_t scheduling_epoch_used_nanoseconds;

    struct core_memory_pool ephemeral_memory;
    struct core_memory_pool outbound_message_memory_pool;

    struct thorium_priority_assigner assigner;

    uint64_t last_wake_up_count;

    uint64_t waiting_start_time;

    uint64_t last_outbound_message_block_operation;

#ifdef THORIUM_WORKER_DEBUG_INJECTION
    int counter_allocated_outbound_buffers;
    int counter_freed_outbound_buffers_from_self;
    int counter_freed_outbound_buffers_from_other_workers;
    int counter_injected_outbound_buffers_other_local_workers;
    int counter_injected_inbound_buffers_from_thorium_core;
#endif

    unsigned int random_seed;
};

void thorium_worker_init(struct thorium_worker *self, int name, struct thorium_node *node);
void thorium_worker_destroy(struct thorium_worker *self);

void thorium_worker_start(struct thorium_worker *self, int processor);
void thorium_worker_stop(struct thorium_worker *self);

void thorium_worker_run(struct thorium_worker *self);

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

struct core_memory_pool *thorium_worker_get_ephemeral_memory(struct thorium_worker *self);

int thorium_worker_dequeue_actor(struct thorium_worker *self, struct thorium_actor **actor);
int thorium_worker_enqueue_inbound_message(struct thorium_worker *self, struct thorium_message *message);
int thorium_worker_enqueue_actor_special(struct thorium_worker *self, struct thorium_actor *actor);

int thorium_worker_enqueue_message(struct thorium_worker *self, struct thorium_message *message);
int thorium_worker_dequeue_message(struct thorium_worker *self, struct thorium_message *message);

void thorium_worker_evict_actor(struct thorium_worker *self, int actor_name);

#ifdef THORIUM_WORKER_ENABLE_LOCK
void thorium_worker_lock(struct thorium_worker *self);
void thorium_worker_unlock(struct thorium_worker *self);
#endif

struct core_map *thorium_worker_get_actors(struct thorium_worker *self);

int thorium_worker_get_sum_of_received_actor_messages(struct thorium_worker *self);
int thorium_worker_get_queued_messages(struct thorium_worker *self);
int thorium_worker_get_production(struct thorium_worker *self, struct thorium_balancer *scheduler);
int thorium_worker_get_producer_count(struct thorium_worker *self, struct thorium_balancer *scheduler);

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

void thorium_worker_print_balance(struct thorium_worker *self);
int thorium_worker_spawn(struct thorium_worker *self, int script);

void thorium_worker_examine(struct thorium_worker *self);
void thorium_worker_enable_profiler(struct thorium_worker *self);
int thorium_worker_get_scheduled_actor_count(struct thorium_worker *self);

void *thorium_worker_allocate(struct thorium_worker *self, size_t count);
int thorium_worker_get_input_message_ring_size(struct thorium_worker *self);

void thorium_worker_set_outbound_message_ring(struct thorium_worker *self, struct core_fast_ring *ring);
void thorium_worker_set_triage_message_ring(struct thorium_worker *self, struct core_fast_ring *ring);
void thorium_worker_set_siblings(struct thorium_worker *self,
                struct thorium_worker *workers, int worker_count);
int thorium_worker_enqueue_outbound_message(struct thorium_worker *self,
                struct thorium_message *message);
struct core_memory_pool *thorium_worker_get_outbound_message_memory_pool(struct thorium_worker *self);
void thorium_worker_send_local_delivery(struct thorium_worker *self, struct thorium_message *message);

int thorium_worker_get_random_number(struct thorium_worker *self);

#endif
