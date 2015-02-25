
#include "worker_pool.h"

#include "message.h"

#include "configuration.h"

/*
#include "message_block.h"
*/

#include "actor.h"
#include "worker.h"
#include "node.h"

#include "thorium_engine.h"

#include "scheduler/migration.h"

#include <core/helpers/vector_helper.h>
#include <core/helpers/statistics.h>
#include <core/helpers/pair.h>

#include <core/structures/set_iterator.h>
#include <core/structures/vector_iterator.h>

#include <core/structures/queue.h>
#include <core/structures/fast_ring.h>

#include <core/system/debugger.h>
#include <core/system/memory.h>

#include <tracepoints/tracepoints.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#define MEMORY_WORKER_POOL_KEY 0x533932bf

/*
#define THORIUM_WORKER_POOL_DEBUG
#define THORIUM_WORKER_POOL_DEBUG_ISSUE_334
*/

#define THORIUM_WORKER_POOL_MESSAGE_SCHEDULING_WINDOW 4

/*
 * Scheduling options.
 */
#define THORIUM_WORKER_POOL_PUSH_WORK_ON_SAME_WORKER
#define THORIUM_WORKER_POOL_FORCE_LAST_WORKER 1

/*
 * Configuration for initial placement.
 */
/*
#define THORIUM_WORKER_POOL_USE_LEAST_BUSY
*/
#define THORIUM_WORKER_POOL_USE_SCRIPT_ROUND_ROBIN

/*
 * Enable the load balancer.
 * The implementation needs to lock everything up,
 * so it is not very good for performance.
 * The next-generation load balancing will use an eviction approach
 * instead.
 */
/*
#define THORIUM_WORKER_POOL_BALANCE
*/

#ifdef THORIUM_WORKER_HAS_OWN_QUEUES
/*
static int thorium_worker_pool_pull_classic(struct thorium_worker_pool *self, struct thorium_message *message);
static void thorium_worker_pool_schedule_work_classic(struct thorium_worker_pool *self, struct biosal_work *work);
*/

#endif

#ifdef THORIUM_WORKER_POOL_USE_COUNT_CACHE
static void thorium_worker_pool_set_cached_value(struct thorium_worker_pool *self, int index, int value);
static inline int thorium_worker_pool_get_cached_value(struct thorium_worker_pool *self, int index);
#endif

/*
static void thorium_worker_pool_wake_up_workers(struct thorium_worker_pool *self);
*/

static void thorium_worker_pool_examine_inbound_queue(struct thorium_worker_pool *self);

static int thorium_worker_pool_give_message_to_worker(struct thorium_worker_pool *self, struct thorium_message *message);

void thorium_worker_pool_init(struct thorium_worker_pool *pool, int workers,
                struct thorium_node *node)
{
#ifdef THORIUM_WORKER_USE_MULTIPLE_PRODUCER_RING
    int outbound_ring_capacity;
#endif

    core_timer_init(&pool->timer);

    pool->debug_mode = 0;
    pool->node = node;
    pool->waiting_is_enabled = 0;

    thorium_balancer_init(&pool->balancer, pool);

    pool->ticks_without_messages = 0;

    core_queue_init(&pool->clean_message_queue, sizeof(struct thorium_message));

    pool->last_warning = 0;
    pool->last_scheduling_warning = 0;

    pool->worker_count = workers;

    pool->worker_for_run = 0;
    pool->worker_for_message = 0;

    /* with only one thread,  the main thread
     * handles everything.
     */
    if (pool->worker_count < 1) {
        thorium_printf("Error: the number of workers must be at least 1.\n");
        core_exit_with_error();
    }

#ifdef THORIUM_WORKER_POOL_HAS_SPECIAL_QUEUES
    biosal_work_queue_init(&pool->work_queue);
    thorium_message_queue_init(&pool->message_queue);
#endif

#ifdef THORIUM_WORKER_ENABLE_WAIT_AND_SIGNAL
    /*
     * Enable the wait/notify algorithm if running on more than
     * one node.
     */
    if (thorium_node_nodes(pool->node) >= 2) {
        pool->waiting_is_enabled = 1;
    }
#endif

#ifdef THORIUM_WORKER_USE_MULTIPLE_PRODUCER_RING
    outbound_ring_capacity = THORIUM_WORKER_POOL_OUTBOUND_RING_SIZE;
    core_fast_ring_init(&pool->outbound_message_ring, outbound_ring_capacity,
                    sizeof(struct thorium_message));
    core_fast_ring_use_multiple_producers(&pool->outbound_message_ring);
    core_fast_ring_init(&pool->triage_message_ring, outbound_ring_capacity,
                    sizeof(struct thorium_message));
    core_fast_ring_use_multiple_producers(&pool->triage_message_ring);
#endif

    thorium_worker_pool_create_workers(pool);

    pool->starting_time = time(NULL);

    /*
    core_queue_init(&pool->scheduled_actor_queue_buffer, sizeof(struct thorium_actor *));
    */
    core_queue_init(&pool->inbound_message_queue_buffer, sizeof(struct thorium_message));

    pool->last_balancing = pool->starting_time;
    pool->last_signal_check = pool->starting_time;

    pool->balance_period = THORIUM_SCHEDULER_PERIOD_IN_SECONDS;

    pool->worker_for_demultiplex = 0;

    pool->last_thorium_report_time = core_timer_get_nanoseconds(&pool->timer);
}

void thorium_worker_pool_destroy(struct thorium_worker_pool *pool)
{
    core_timer_destroy(&pool->timer);

    thorium_balancer_destroy(&pool->balancer);

    thorium_worker_pool_delete_workers(pool);

    pool->node = NULL;

    core_queue_destroy(&pool->inbound_message_queue_buffer);
    /*
    core_queue_destroy(&pool->scheduled_actor_queue_buffer);
    */
    core_queue_destroy(&pool->clean_message_queue);

#ifdef THORIUM_WORKER_USE_MULTIPLE_PRODUCER_RING
    core_fast_ring_destroy(&pool->outbound_message_ring);
    core_fast_ring_destroy(&pool->triage_message_ring);
#endif
}

void thorium_worker_pool_delete_workers(struct thorium_worker_pool *pool)
{
    int i = 0;
    struct thorium_worker *worker;

    if (pool->worker_count <= 0) {
        return;
    }

    for (i = 0; i < pool->worker_count; i++) {
        worker = thorium_worker_pool_get_worker(pool, i);

#if 0
        thorium_printf("worker/%d loop_load %f\n", thorium_worker_name(worker),
                    thorium_worker_get_loop_load(worker));
#endif

        thorium_worker_destroy(worker);
    }

    core_vector_destroy(&pool->worker_array);

#ifdef THORIUM_WORKER_POOL_USE_COUNT_CACHE
    core_vector_destroy(&pool->message_count_cache);
#endif
}

void thorium_worker_pool_create_workers(struct thorium_worker_pool *pool)
{
    int i;
    struct thorium_worker *worker;

    if (pool->worker_count <= 0) {
        return;
    }

    core_vector_init(&pool->worker_array, sizeof(struct thorium_worker));
#ifdef THORIUM_WORKER_POOL_USE_COUNT_CACHE
    core_vector_init(&pool->message_count_cache, sizeof(int));
#endif

    core_vector_resize(&pool->worker_array, pool->worker_count);
#ifdef THORIUM_WORKER_POOL_USE_COUNT_CACHE
    core_vector_resize(&pool->message_count_cache, pool->worker_count);
#endif

    pool->worker_cache = (struct thorium_worker *)core_vector_at(&pool->worker_array, 0);
#ifdef THORIUM_WORKER_POOL_USE_COUNT_CACHE
    pool->message_cache = (int *)core_vector_at(&pool->message_count_cache, 0);
#endif

    for (i = 0; i < pool->worker_count; i++) {

        worker = thorium_worker_pool_get_worker(pool, i);
        thorium_worker_init(worker, i, pool->node);

#ifdef THORIUM_WORKER_USE_MULTIPLE_PRODUCER_RING
        /*
         * The worker will push to this ring.
         */
        thorium_worker_set_outbound_message_ring(worker, &pool->outbound_message_ring);
        thorium_worker_set_triage_message_ring(worker, &pool->triage_message_ring);
#endif

#ifdef THORIUM_WORKER_ENABLE_WAIT_AND_SIGNAL
        if (pool->waiting_is_enabled) {
            thorium_worker_enable_waiting(worker);
        }
#endif

#ifdef THORIUM_WORKER_POOL_USE_COUNT_CACHE
        core_vector_set_int(&pool->message_count_cache, i, 0);
#endif

        thorium_worker_set_siblings(worker, pool->worker_cache,
                        pool->worker_count);
    }
}

void thorium_worker_pool_start(struct thorium_worker_pool *pool)
{
    int i;
    int processor;

    /* start workers
     *
     * we start at 1 because the thread 0 is
     * used by the main thread...
     */
    for (i = 0; i < pool->worker_count; i++) {
        processor = i;

        if (thorium_node_nodes(pool->node) != 1) {
            processor = -1;
        }

        thorium_worker_start(thorium_worker_pool_get_worker(pool, i), processor);
    }
}

void thorium_worker_pool_run(struct thorium_worker_pool *pool)
{
    /* make the thread work (this is the main thread) */
    thorium_worker_run(thorium_worker_pool_select_worker_for_run(pool));
}

void thorium_worker_pool_stop(struct thorium_worker_pool *pool)
{
    int i;
    /*
     * stop workers
     */

#ifdef THORIUM_WORKER_POOL_DEBUG
    thorium_printf("Stop workers\n");
#endif

    for (i = 0; i < pool->worker_count; i++) {
        thorium_worker_stop(thorium_worker_pool_get_worker(pool, i));
    }
}

struct thorium_worker *thorium_worker_pool_select_worker_for_run(struct thorium_worker_pool *pool)
{
    int index;

    index = pool->worker_for_run;
    return thorium_worker_pool_get_worker(pool, index);
}

/*
 * All messages go through here.
 */
int thorium_worker_pool_enqueue_message(struct thorium_worker_pool *pool, struct thorium_message *message)
{
    int value;

    tracepoint(thorium_message, worker_pool_enqueue, message);

    value = thorium_worker_pool_give_message_to_worker(pool, message);

    return value;
}

int thorium_worker_pool_worker_count(struct thorium_worker_pool *pool)
{
    return pool->worker_count;
}

void thorium_worker_pool_print_load(struct thorium_worker_pool *self, int type)
{
    int count;
    int i;
    float epoch_load;
    struct thorium_worker *worker;
    float loop_load;

    uint64_t epoch_wake_up_count;
    uint64_t loop_wake_up_count;
    /*
    int scheduling_score;
    */
    int node_name;

    char *buffer;
    char *buffer_for_wake_up_events;
    char *buffer_for_future_timeline;
    char *buffer_for_traffic_aggregation;
    char *buffer_for_input_throughput;
    char *buffer_for_output_throughput;
    char *buffer_for_worker_frequency;

    int allocated;

    int offset;
    int offset_for_wake_up;
    int offset_for_future;
    int offset_for_traffic_reduction;
    int offset_for_input_throughput;
    int offset_for_output_throughput;
    int offset_for_frequency;

    int extra;
    time_t current_time;
    int elapsed;
    float selected_load;
    uint64_t selected_wake_up_count;
    float sum;
    /*
    char loop[] = "COMPUTATION";
    char epoch[] = "EPOCH";
    */
    float load;
    double input_throughput;
    double output_throughput;
    double worker_frequency;
    uint64_t nanoseconds;
    uint64_t elapsed_nanoseconds;


    if (type == THORIUM_WORKER_POOL_LOAD_LOOP) {
    } else if (type == THORIUM_WORKER_POOL_LOAD_EPOCH) {
    } else {
        return;
    }

    current_time = time(NULL);
    elapsed = current_time - self->starting_time;

    if (elapsed == 0)
        return;

    nanoseconds = core_timer_get_nanoseconds(&self->timer);
    elapsed_nanoseconds = nanoseconds - self->last_thorium_report_time;
    self->last_thorium_report_time = nanoseconds;

    extra = 100;

    count = thorium_worker_pool_worker_count(self);
    allocated = count * 20 + 20 + extra;

    buffer = core_memory_allocate(allocated, MEMORY_WORKER_POOL_KEY);
    buffer_for_wake_up_events = core_memory_allocate(allocated, MEMORY_WORKER_POOL_KEY);
    buffer_for_future_timeline = core_memory_allocate(allocated, MEMORY_WORKER_POOL_KEY);
    buffer_for_traffic_aggregation = core_memory_allocate(allocated, MEMORY_WORKER_POOL_KEY);
    buffer_for_input_throughput = core_memory_allocate(allocated, MEMORY_WORKER_POOL_KEY);
    buffer_for_output_throughput= core_memory_allocate(allocated, MEMORY_WORKER_POOL_KEY);
    buffer_for_worker_frequency = core_memory_allocate(allocated, MEMORY_WORKER_POOL_KEY);

    node_name = thorium_node_name(self->node);
    offset = 0;
    offset_for_wake_up = 0;
    offset_for_future = 0;
    offset_for_traffic_reduction = 0;
    offset_for_input_throughput = 0;
    offset_for_output_throughput = 0;
    offset_for_frequency = 0;

    i = 0;
    sum = 0;

    while (i < count && offset + extra < allocated) {

        worker = thorium_worker_pool_get_worker(self, i);

        epoch_load = thorium_worker_get_epoch_load(worker);
        loop_load = thorium_worker_get_loop_load(worker);
        epoch_wake_up_count = thorium_worker_get_epoch_wake_up_count(worker);
        loop_wake_up_count = thorium_worker_get_loop_wake_up_count(worker);

        input_throughput = thorium_worker_get_event_counter(worker, THORIUM_EVENT_ACTOR_RECEIVE);
        input_throughput -= thorium_worker_get_last_event_counter(worker, THORIUM_EVENT_ACTOR_RECEIVE);
        input_throughput /= (0.0 + elapsed_nanoseconds) / (1000 * 1000 * 1000);
        thorium_worker_set_last_event_counter(worker, THORIUM_EVENT_ACTOR_RECEIVE);

        output_throughput = thorium_worker_get_event_counter(worker, THORIUM_EVENT_ACTOR_SEND);
        output_throughput -= thorium_worker_get_last_event_counter(worker, THORIUM_EVENT_ACTOR_SEND);
        output_throughput /= (0.0 + elapsed_nanoseconds) / (1000 * 1000 * 1000);
        thorium_worker_set_last_event_counter(worker, THORIUM_EVENT_ACTOR_SEND);

        worker_frequency = thorium_worker_get_event_counter(worker, THORIUM_EVENT_WORKER_TICK);
        worker_frequency -= thorium_worker_get_last_event_counter(worker, THORIUM_EVENT_WORKER_TICK);
        worker_frequency /= (0.0 + elapsed_nanoseconds) / (1000 * 1000 * 1000);
        thorium_worker_set_last_event_counter(worker, THORIUM_EVENT_WORKER_TICK);

        selected_load = epoch_load;
        selected_wake_up_count = epoch_wake_up_count;

        if (type == THORIUM_WORKER_POOL_LOAD_EPOCH) {
            selected_load = epoch_load;
            selected_wake_up_count = epoch_wake_up_count;

        } else if (type == THORIUM_WORKER_POOL_LOAD_LOOP) {
            selected_load = loop_load;
            selected_wake_up_count = loop_wake_up_count;
        }

        /*
        offset += sprintf(buffer + offset, " [%d %d %.2f]", i,
                        scheduling_score,
                        selected_load);
                        */
        offset += sprintf(buffer + offset, " %.2f",
                        selected_load);

        offset_for_wake_up += sprintf(buffer_for_wake_up_events + offset_for_wake_up, " %" PRIu64 "",
                        selected_wake_up_count);

        offset_for_future += sprintf(buffer_for_future_timeline + offset_for_future, " %d",
                        thorium_worker_get_scheduled_actor_count(worker));

        offset_for_traffic_reduction += sprintf(buffer_for_traffic_aggregation + offset_for_traffic_reduction,
                        " %d", thorium_worker_get_epoch_degree_of_aggregation(worker));

        offset_for_input_throughput += sprintf(buffer_for_input_throughput + offset_for_input_throughput,
                        " %.2f", input_throughput);
        offset_for_output_throughput += sprintf(buffer_for_output_throughput + offset_for_output_throughput,
                        " %.2f", output_throughput);

        offset_for_frequency += sprintf(buffer_for_worker_frequency + offset_for_frequency,
                        " %.2f", worker_frequency);

        sum += selected_load;

        ++i;
    }

    load = sum / count;

    thorium_printf("[thorium] node %d LOAD %d s %.2f/%d (%.2f)%s\n",
                    node_name, elapsed,
                    sum, count, load, buffer);

    if (type == THORIUM_WORKER_POOL_LOAD_EPOCH) {
        thorium_printf("[thorium] node %d FUTURE_TIMELINE %d s %s\n",
                    node_name, elapsed,
                    buffer_for_future_timeline);

        thorium_printf("[thorium] node %d INPUT_MPS %d s%s\n",
                    node_name, elapsed,
                    buffer_for_input_throughput);

        thorium_printf("[thorium] node %d OUTPUT_MPS %d s%s\n",
                    node_name, elapsed,
                    buffer_for_output_throughput);

        thorium_printf("[thorium] node %d HZ %d s%s\n",
                        node_name, elapsed, buffer_for_worker_frequency);
    }

#ifdef THORIUM_WORKER_ENABLE_WAIT_AND_SIGNAL
    thorium_printf("[thorium] node %d %s WAKE_UP_COUNT %d s %s\n",
                    node_name,
                    description, elapsed,
                    buffer_for_wake_up_events);
#endif

    if (type == THORIUM_WORKER_POOL_LOAD_EPOCH) {
        thorium_printf("[thorium] node %d DOA %d s%s\n",
                    node_name, elapsed,
                    buffer_for_traffic_aggregation);
    }

    core_memory_free(buffer, MEMORY_WORKER_POOL_KEY);
    core_memory_free(buffer_for_wake_up_events, MEMORY_WORKER_POOL_KEY);
    core_memory_free(buffer_for_future_timeline, MEMORY_WORKER_POOL_KEY);
    core_memory_free(buffer_for_traffic_aggregation, MEMORY_WORKER_POOL_KEY);
    core_memory_free(buffer_for_input_throughput, MEMORY_WORKER_POOL_KEY);
    core_memory_free(buffer_for_output_throughput, MEMORY_WORKER_POOL_KEY);
    core_memory_free(buffer_for_worker_frequency, MEMORY_WORKER_POOL_KEY);
}

void thorium_worker_pool_toggle_debug_mode(struct thorium_worker_pool *self)
{
    self->debug_mode = !self->debug_mode;
}

float thorium_worker_pool_get_computation_load(struct thorium_worker_pool *pool)
{
    double load;
    struct thorium_worker *worker;
    int i;

    load = 0;

    for (i = 0; i < pool->worker_count; i++) {
        worker = thorium_worker_pool_get_worker(pool, i);
        load += thorium_worker_get_loop_load(worker);
    }

    if (pool->worker_count != 0) {
        load /= pool->worker_count;
    }

    return load;
}

struct thorium_node *thorium_worker_pool_get_node(struct thorium_worker_pool *pool)
{
    return pool->node;
}

static int thorium_worker_pool_give_message_to_worker(struct thorium_worker_pool *pool, struct thorium_message *message)
{
    struct thorium_worker *affinity_worker;
    int worker_index;
    int name;
    int action;

#ifdef CHECK_IF_ACTOR_IS_DEAD
    int dead;
    struct thorium_actor *actor;
#endif

    action = thorium_message_action(message);

    /*
     * Inject multiplexed message in one of the worker for
     * distributed demultiplexing.
     */
    if (action == ACTION_MULTIPLEXER_MESSAGE) {

        /*
         * Use a round-robin for messages to demultiplex.
         */
        worker_index = pool->worker_for_demultiplex;

        ++pool->worker_for_demultiplex;
        if (pool->worker_for_demultiplex == pool->worker_count)
            pool->worker_for_demultiplex = 0;

        affinity_worker = thorium_worker_pool_get_worker(pool, worker_index);

        if (!thorium_worker_enqueue_inbound_message(affinity_worker, message)) {

            core_queue_enqueue(&pool->inbound_message_queue_buffer, message);
        }

        return 1;
    }

    /*
    void *buffer;

    buffer = thorium_message_buffer(message);
    */
    name = thorium_message_destination(message);

#ifdef CHECK_IF_ACTOR_IS_DEAD
    actor = thorium_node_get_actor_from_name(pool->node, name);

    if (actor == NULL) {
#ifdef THORIUM_WORKER_POOL_DEBUG_DEAD_CHANNEL
        thorium_printf("DEAD LETTER CHANNEL...\n");
#endif

        core_queue_enqueue(&pool->clean_message_queue, message);

        return 0;
    }

    dead = thorium_actor_dead(actor);

    /* If the actor is dead, don't use it.
     */
    if (dead) {

        core_queue_enqueue(&pool->clean_message_queue, message);

        return 0;
    }
#endif

    /* Check if the actor is already assigned to a worker
     */
    worker_index = thorium_balancer_get_actor_worker(&pool->balancer, name);

    /* If not, ask the scheduler to assign the actor to a worker
     */
    if (worker_index == THORIUM_WORKER_NONE) {

        thorium_worker_pool_assign_worker_to_actor(pool, name);
        worker_index = thorium_balancer_get_actor_worker(&pool->balancer, name);
    }

    if (worker_index == THORIUM_WORKER_NONE) {

#ifdef DEBUG_DEAD_CHANNEL
        thorium_printf("Warning, can not deliver action %x to %d\n",
                        thorium_message_action(message), name);
#endif

        /*
         * Deliver the message somewhere anyway to recycle the message.
         */
        worker_index = 0;
    }
    affinity_worker = thorium_worker_pool_get_worker(pool, worker_index);

    /* give the message to the actor
     */
    if (!thorium_worker_enqueue_inbound_message(affinity_worker, message)) {

#ifdef THORIUM_WORKER_POOL_DEBUG_MESSAGE_BUFFERING
        thorium_printf("DEBUG897 could not enqueue message, buffering...\n");
#endif

        core_queue_enqueue(&pool->inbound_message_queue_buffer, message);

        /*
         * It would be interesting to see what is in the queue,
         * but this would produce huge log files.
         */
#ifdef INSPECT_INBOUND_QUEUE
        if (core_queue_size(&pool->inbound_message_queue_buffer) >= 10000) {
            thorium_worker_pool_examine_inbound_queue(pool);
            CORE_DEBUGGER_ASSERT(0);
        }
#endif

#if 0
    /*
     * The code block below was used for pushing actors to schedule to
     * workers. The model has been refined and now only inbound messages
     * are pushed to workers.
     */
    } else {
        /*
         * At this point, the message has been pushed to the actor.
         * Now, the actor must be scheduled on a worker.
         */
/*
        thorium_printf("DEBUG message was enqueued in actor mailbox\n");
        */

        /* Check if the actor is already assigned to a worker
         */
        worker_index = thorium_balancer_get_actor_worker(&pool->balancer, name);

        /* If not, ask the scheduler to assign the actor to a worker
         */
        if (worker_index < 0) {

            thorium_worker_pool_assign_worker_to_actor(pool, name);
            worker_index = thorium_balancer_get_actor_worker(&pool->balancer, name);
        }

        affinity_worker = thorium_worker_pool_get_worker(pool, worker_index);

        /*
        thorium_printf("DEBUG actor has an assigned worker\n");
        */

        /*
         * Push the actor on the scheduling queue of the worker.
         * If that fails, queue the actor.
         */
        if (!thorium_worker_enqueue_actor(affinity_worker, actor)) {
            core_queue_enqueue(&pool->scheduled_actor_queue_buffer, &actor);
        }
#endif
    }

    return 1;
}

void thorium_worker_pool_work(struct thorium_worker_pool *pool)
{
    struct thorium_message other_message;

    /*
    struct thorium_actor *actor;
    int worker_index;
    struct thorium_worker *worker;
    int name;
    */

#ifdef THORIUM_WORKER_POOL_BALANCE
#endif

    /* If there are messages in the inbound message buffer,
     * Try to give  them too.
     */
    if (core_queue_dequeue(&pool->inbound_message_queue_buffer, &other_message)) {
        thorium_worker_pool_give_message_to_worker(pool, &other_message);
    }

    if (core_fast_ring_pop_from_consumer(&pool->triage_message_ring,
                            &other_message)) {

        core_queue_enqueue(&pool->clean_message_queue, &other_message);
    }

#if 0
    /* Try to dequeue an actor for scheduling
     */
    if (core_queue_dequeue(&pool->scheduled_actor_queue_buffer, &actor)) {

        name = thorium_actor_name(actor);
        worker_index = thorium_balancer_get_actor_worker(&pool->balancer, name);

        if (worker_index < 0) {
            /*
             * This case is very rare, but will happen since the number of
             * messages and the number of actors are both very large.
             *
             * This case will happen if these 3 conditions are satisfied:
             *
             * A message was not queued in the mailbox of the actor X because its mailbox was full.
             * The message, in that case, will be queued in the actor mailbox at a later time.
             * But this code path will not queue the actor on any worker since the actor has yet.
             * But the actor do have messages, but the problem is that all the actor scheduling queue
             * on every worker are full.
             */
#ifdef THORIUM_WORKER_POOL_DEBUG_ACTOR_ASSIGNMENT_PROBLEM
            thorium_printf("Notice: actor %d has no assigned worker\n", name);
#endif
            thorium_worker_pool_assign_worker_to_actor(pool, name);
            worker_index = thorium_balancer_get_actor_worker(&pool->balancer, name);
        }

        worker = thorium_worker_pool_get_worker(pool, worker_index);

        if (!thorium_worker_enqueue_actor(worker, actor)) {
            core_queue_enqueue(&pool->scheduled_actor_queue_buffer, &actor);
        }
    }
#endif

#if 0
    thorium_printf("DEBUG pool receives message for actor %d\n",
                    destination);
#endif

#ifdef THORIUM_WORKER_POOL_BALANCE
    /* balance the pool regularly
     */


    if (current_time - pool->last_balancing >= pool->balance_period) {
        thorium_balancer_balance(&pool->scheduler);

        pool->last_balancing = current_time;
    }
#endif

#ifdef THORIUM_WORKER_ENABLE_WAIT_AND_SIGNAL
    /*
     * If waiting is enabled, it is required to wake up workers
     * once in a while because of the ordering
     * of events for the actor scheduling queue
     */
    /* Example, thorium_worker_signal is called just a bit before
     * thorium_worker_wait.
     *
     */

    if (pool->waiting_is_enabled) {
        thorium_worker_pool_wake_up_workers(pool);
    }
#endif
}

void thorium_worker_pool_assign_worker_to_actor(struct thorium_worker_pool *pool, int name)
{
    int worker_index;

#ifdef THORIUM_WORKER_POOL_USE_LEAST_BUSY
    int score;
#endif

#ifdef THORIUM_WORKER_POOL_USE_SCRIPT_ROUND_ROBIN
    int script;
    struct thorium_actor *actor;
#endif

                /*
    thorium_printf("DEBUG Needs to do actor placement\n");
    */
    /* assign this actor to the least busy actor
     */

    worker_index = THORIUM_WORKER_NONE;

#ifdef THORIUM_WORKER_POOL_USE_LEAST_BUSY
    worker_index = thorium_balancer_select_worker_least_busy(&pool->scheduler, &score);

#elif defined(THORIUM_WORKER_POOL_USE_SCRIPT_ROUND_ROBIN)
    actor = thorium_node_get_actor_from_name(pool->node, name);

    /*
     * Somehow this actor dead a while ago.
     */
    if (actor == NULL) {
/*
        thorium_printf("Warning: actor %d does not exist\n", name);
        */
        return;
    }

    /* The actor can't be dead if it does not have an initial
     * placement...
     */
    CORE_DEBUGGER_ASSERT(actor != NULL);

    script = thorium_actor_script(actor);

    worker_index = thorium_balancer_select_worker_script_round_robin(&pool->balancer, script);
#endif

    CORE_DEBUGGER_ASSERT(worker_index >= 0);

#ifdef THORIUM_WORKER_POOL_DEBUG
    thorium_printf("ASSIGNING %d to %d\n", name, worker_index);
#endif

    thorium_balancer_set_actor_worker(&pool->balancer, name, worker_index);
}

float thorium_worker_pool_get_current_load(struct thorium_worker_pool *pool)
{
    float load;
    int workers;
    int i;

    workers = thorium_worker_pool_worker_count(pool);

    load = 0;
    i = 0;

    while (i < workers) {

        load += thorium_worker_get_epoch_load(thorium_worker_pool_get_worker(pool, i));

        ++i;
    }

    load /= workers;

    return load;
}

int thorium_worker_pool_next_worker(struct thorium_worker_pool *pool, int worker)
{
    worker++;

    /* wrap the counter
     */
    if (worker == pool->worker_count) {
        worker = 0;
    }

    return worker;
}

struct thorium_worker *thorium_worker_pool_get_worker(
                struct thorium_worker_pool *self, int index)
{
    return self->worker_cache + index;
}

#ifdef THORIUM_WORKER_POOL_USE_COUNT_CACHE
static void thorium_worker_pool_set_cached_value(struct thorium_worker_pool *self, int index, int value)
{
    self->message_cache[index] = value;
}

static inline int thorium_worker_pool_get_cached_value(struct thorium_worker_pool *self, int index)
{
    return self->message_cache[index];
}
#endif

struct thorium_worker *thorium_worker_pool_select_worker_for_message_round_robin(struct thorium_worker_pool *self)
{
    struct thorium_worker *worker;

    worker = core_vector_at(&self->worker_array, self->worker_for_message);
    ++self->worker_for_message;
    if (self->worker_for_message == self->worker_count)
        self->worker_for_message = 0;

    return worker;
}

/* select a worker to pull from */
struct thorium_worker *thorium_worker_pool_select_worker_for_message(struct thorium_worker_pool *pool)
{
    int index;
    int i;
    int score;
    struct thorium_worker *worker;
    int attempts;
    struct thorium_worker *best_worker;
    int best_score;
#ifdef THORIUM_WORKER_POOL_USE_COUNT_CACHE
    int best_index;

    best_index = THORIUM_WORKER_NONE;
#endif
    best_score = 0;
    best_worker = NULL;

    i = 0;
    attempts = THORIUM_WORKER_POOL_MESSAGE_SCHEDULING_WINDOW;

    /* select thet worker with the most messages in the window.
     */
    while (i < attempts) {

        index = pool->worker_for_message;
        pool->worker_for_message = thorium_worker_pool_next_worker(pool, index);
        worker = thorium_worker_pool_get_worker(pool, index);
#ifdef THORIUM_WORKER_POOL_USE_COUNT_CACHE
        score = thorium_worker_pool_get_cached_value(pool, index);
#else
        score = -1;
#endif

        /* Update the cache.
         * This is expensive because it will touch the cache line.
         * Only the worker is increasing the number of messages, and
         * only the worker pool is decreasing it.
         * As long as the cached value is greater than 0, then there is
         * definitely something to pull without the need
         * to break the CPU cache line
         *
         */
        /* always update cache because otherwise there will be
         * starvation
         */
        if (1 || score == 0) {
            score = thorium_worker_get_message_production_score(worker);
#ifdef THORIUM_WORKER_POOL_USE_COUNT_CACHE
            thorium_worker_pool_set_cached_value(pool, index, score);
#endif
        }

        if (best_worker == NULL || score > best_score) {
            best_worker = worker;
            best_score = score;
#ifdef THORIUM_WORKER_POOL_USE_COUNT_CACHE
            best_index = index;
#endif
        }

        ++i;
    }

#ifdef THORIUM_WORKER_POOL_USE_COUNT_CACHE
    /* Update the cached value for the winning worker to have an
     * accurate value for this worker.
     */
    core_vector_set_int(&pool->message_count_cache, best_index, best_score - 1);
#endif

    return best_worker;
}

#if 0
static int thorium_worker_pool_pull_classic(struct thorium_worker_pool *self, struct thorium_message *message)
{
    struct thorium_worker *worker;
    int answer;
    int size;

    size = thorium_worker_pool_worker_count(self);
    answer = 0;

    /*
     * Try to find an outbound buffer.
     *
     * To do this, loop over at most all the workers.
     *
     * The idea here is that it is better to always have a outbound
     * message generated by this call.
     */
    while (!answer && size > 0) {
        worker = thorium_worker_pool_select_worker_for_message_round_robin(self);
        answer = thorium_worker_dequeue_message(worker, message);

        --size;
    }

    return answer;
}
#endif

int thorium_worker_pool_dequeue_message(struct thorium_worker_pool *pool, struct thorium_message *message)
{
    int answer;

#ifdef THORIUM_WORKER_USE_MULTIPLE_PRODUCER_RING
    /*
     * Pull message from the multiple-producer ring.
     */
    answer = core_fast_ring_pop_from_consumer(&pool->outbound_message_ring, message);

    if (answer) {
        tracepoint(thorium_node, worker_pool_dequeue, pool->node->name,
                        core_fast_ring_size_from_producer(&pool->outbound_message_ring));
    }
#else

    /*
     * Pull message using a round-robin scheme on the workers' rings.
     */
    answer = thorium_worker_pool_pull_classic(pool, message);
#endif

#if 0
    /*
     * Possibly record a tracepoint.
     */
    if (answer) {
        tracepoint(thorium_message, worker_pool_dequeue, message);
    }
#endif

    return answer;
}

#if 0
static void thorium_worker_pool_wake_up_workers(struct thorium_worker_pool *pool)
{
    float load;
    int i;
    time_t current_time;
    int period;
    struct thorium_worker *worker;
    int elapsed;

    period = 1;

    /*
     * Send a signal to any worker without activity.
     * This is required because a thread can go to sleep after the signal was sent
     * (the first signal).
     */

    i = 0;

    current_time = time(NULL);

    if (current_time - pool->last_signal_check >= period) {
        while (i < pool->worker_count) {

            worker = thorium_worker_pool_get_worker(pool, i);
            load = thorium_worker_get_epoch_load(worker);
            elapsed = current_time - thorium_worker_get_last_report_time(worker);

            /*
             * Wake up the worker (for instance, worker/8)
             * so that it pulls something.
             */
            if (load < 0.1 || elapsed >= 1) {
                thorium_worker_signal(worker);
            }

            ++i;
        }
        pool->last_signal_check = current_time;
    }
}
#endif

int thorium_worker_pool_dequeue_message_for_triage(struct thorium_worker_pool *self,
                struct thorium_message *message)
{
    return core_queue_dequeue(&self->clean_message_queue, message);
}

void thorium_worker_pool_examine(struct thorium_worker_pool *self)
{
    int i;
    int size;
    struct thorium_worker *worker;

    i = 0;
    size = thorium_worker_pool_worker_count(self);

    /*
    thorium_printf("QUEUE Name= scheduled_actor_queue_buffer size= %d\n",
                    core_queue_size(&self->scheduled_actor_queue_buffer));
                    */

    thorium_worker_pool_examine_inbound_queue(self);

    thorium_printf("QUEUE Name= clean_message_queue size= %d\n",
                    core_queue_size(&self->clean_message_queue));

    for (i = 0; i < size; ++i) {

        worker = thorium_worker_pool_get_worker(self, i);

        thorium_worker_examine(worker);
    }
}

void thorium_worker_pool_enable_profiler(struct thorium_worker_pool *self)
{
    int i;
    int size;
    struct thorium_worker *worker;

    i = 0;
    size = thorium_worker_pool_worker_count(self);

    for (i = 0; i < size; ++i) {

        worker = thorium_worker_pool_get_worker(self, i);

        thorium_worker_enable_profiler(worker);
    }
}

int thorium_worker_pool_buffered_message_count(struct thorium_worker_pool *self)
{
    return core_queue_size(&self->inbound_message_queue_buffer);
}

static void thorium_worker_pool_examine_inbound_queue(struct thorium_worker_pool *self)
{
    int queue_size;
    struct thorium_message message;
    int i;

    queue_size = core_queue_size(&self->inbound_message_queue_buffer);
    thorium_printf("QUEUE Name= inbound_message_queue_buffer size= %d\n",
                    queue_size);

    /*
     * Print the content of the queue.
     */
    if (queue_size > 4) {

        i = 0;
        while (i < queue_size) {

            core_queue_dequeue(&self->inbound_message_queue_buffer, &message);
            thorium_message_print(&message);
            core_queue_enqueue(&self->inbound_message_queue_buffer, &message);

            ++i;
        }
    }
}

int thorium_worker_pool_outbound_ring_size(struct thorium_worker_pool *self)
{
    return core_fast_ring_size_from_consumer(&self->outbound_message_ring);
}

int thorium_worker_pool_triage_message_queue_size(struct thorium_worker_pool *self)
{
    return core_queue_size(&self->clean_message_queue);
}
