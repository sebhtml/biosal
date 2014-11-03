
#include "worker.h"

#include "message.h"
/*
#include "message_block.h"
*/
#include "node.h"

#include "configuration.h"

#include "worker_debugger.h"

#include "scheduler/balancer.h"

#include <core/structures/map.h>
#include <core/structures/vector.h>
#include <core/structures/vector_iterator.h>
#include <core/structures/map_iterator.h>
#include <core/structures/set_iterator.h>

#include <core/constants.h>

#include <core/helpers/vector_helper.h>
#include <core/helpers/bitmap.h>

#include <core/system/command.h>
#include <core/system/memory.h>
#include <core/system/timer.h>
#include <core/system/debugger.h>

#include <performance/tracepoints/tracepoints.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h> /*getpid */

/*#define THORIUM_WORKER_DEBUG
  */

#undef CORE_DEBUGGER_JITTER_DETECTION_START
#undef CORE_DEBUGGER_JITTER_DETECTION_END
#define CORE_DEBUGGER_JITTER_DETECTION_START(name)
#define CORE_DEBUGGER_JITTER_DETECTION_END(name, actor_time)

/*
 * Display a warning when a ring becomes full.
 */
/*
#define SHOW_FULL_RING_WARNINGS
 */

#define MEMORY_POOL_NAME_WORKER_EPHEMERAL  0x2ee1c5a6
#define MEMORY_POOL_NAME_WORKER_OUTBOUND   0x46d316e4

/*
#define THORIUM_WORKER_DEBUG_MEMORY
#define THORIUM_BUG_594
*/

#define THORIUM_WORKER_UNPRODUCTIVE_TICK_LIMIT 256

#define THORIUM_GRANULARITY_WARNING_THRESHOLD (500 * 1000)

/*
 * The amount required idle time to go to sleep
 * for a worker.
 *
 * The current value is high because it is assumed
 * that they are usually busy.
 *
 * TODO lower this value.
 */
/*#define THORIUM_WORKER_UNPRODUCTIVE_MICROSECONDS_FOR_WAIT (64 * 1000)*/

/*
 * 30 seconds.
 */
#define THORIUM_WORKER_UNPRODUCTIVE_MICROSECONDS_FOR_WAIT (30 * 1000 * 1000)

/*
 * Worker flags.
 */
#define FLAG_DEBUG_ACTORS               0
#define FLAG_DEAD                       1
#define FLAG_DEBUG                      2
#define FLAG_BUSY                       3
#define FLAG_ENABLE_ACTOR_LOAD_PROFILER 4
#define FLAG_ENABLE_WAIT                5
#define FLAG_OUTPUT_OUTBOUND_MESSAGE_RING_IS_FULL        6

#define DEBUG_WORKER_OPTION "-debug-worker"

/*
#define THORIUM_WORKER_DEBUG_WAIT_SIGNAL
*/

/*
 * Print scheduling queue for debugging purposes
 */
/*
#define THORIUM_WORKER_PRINT_SCHEDULING_QUEUE
*/

/* Debug symmetric actor placement
 */
/*
#define THORIUM_WORKER_DEBUG_SYMMETRIC_PLACEMENT
*/

/*
 * Send messages to local actor directly.
 */
/*
*/
#define THORIUM_WORKER_SEND_TO_LOCAL_ACTOR

static void thorium_worker_work(struct thorium_worker *self, struct thorium_actor *actor);
/*
void thorium_worker_flush_outbound_message_block(struct thorium_worker *self);
*/

/*
 * Backoff stuff.
 */
static void thorium_worker_configure_backoff(struct thorium_worker *self);
static void thorium_worker_activate_backoff(struct thorium_worker *self);
static void thorium_worker_do_backoff(struct thorium_worker *self);

static void thorium_worker_schedule_actor(struct thorium_worker *self, struct thorium_actor *actor);
static int thorium_worker_has_actor(struct thorium_worker *self, int actor);

static int thorium_worker_publish_message(struct thorium_worker *self, struct thorium_message *message);

static int thorium_worker_enqueue_message_for_multiplexer(struct thorium_worker *self,
                struct thorium_message *message);

static int thorium_worker_enqueue_message_for_triage(struct thorium_worker *worker, struct thorium_message *message);
/*
static int thorium_worker_dequeue_message_for_triage(struct thorium_worker *worker, struct thorium_message *message);
*/

void thorium_worker_init(struct thorium_worker *worker, int name, struct thorium_node *node)
{
    int capacity;
    int ephemeral_memory_block_size;
    int injected_buffer_ring_size;
    int argc;
    char **argv;
#ifndef THORIUM_WORKER_USE_MULTIPLE_PRODUCER_RING
    int outbound_ring_capacity;
#endif

    /*
    thorium_message_block_init(&worker->message_block);
    */

    worker->tick_count = 0;

    argc = thorium_node_argc(node);
    argv = thorium_node_argv(node);

#ifdef THORIUM_WORKER_DEBUG_INJECTION
    worker->counter_allocated_outbound_buffers = 0;
    worker->counter_freed_outbound_buffers_from_self = 0;
    worker->counter_freed_outbound_buffers_from_other_workers = 0;
    worker->counter_injected_outbound_buffers_other_local_workers= 0;
    worker->counter_injected_inbound_buffers_from_thorium_core = 0;
#endif

    core_map_init(&worker->actor_received_messages, sizeof(int), sizeof(int));

    worker->waiting_start_time = 0;

    core_timer_init(&worker->timer);
    capacity = THORIUM_WORKER_RING_CAPACITY;
    /*worker->work_queue = work_queue;*/
    worker->node = node;
    worker->name = name;

    worker->random_seed = name * getpid();

    CORE_BITMAP_CLEAR(worker->flags);
    CORE_BITMAP_CLEAR_BIT(worker->flags, FLAG_DEAD);
    worker->last_warning = 0;

    worker->last_wake_up_count = 0;

    /*worker->work_queue = &worker->works;*/

    /* There are two options:
     * 1. enable atomic operations for change visibility
     * 2. Use volatile head and tail.
     */
    core_fast_ring_init(&worker->input_inbound_message_ring, capacity, sizeof(struct thorium_message));
    core_fast_ring_use_multiple_producers(&worker->input_inbound_message_ring);

    core_fast_queue_init(&worker->input_inbound_message_queue, sizeof(struct thorium_message));

    /*
     * Multiplexer stuff
     */
    core_fast_ring_init(&worker->input_message_ring_for_multiplexer, capacity, sizeof(struct thorium_message));
    core_fast_ring_use_multiple_producers(&worker->input_message_ring_for_multiplexer);

    core_fast_queue_init(&worker->output_outbound_message_queue_for_multiplexer, sizeof(struct thorium_message));

#ifdef THORIUM_NODE_INJECT_CLEAN_WORKER_BUFFERS
    injected_buffer_ring_size = capacity;
    core_fast_ring_init(&worker->input_clean_outbound_buffer_ring,
                    injected_buffer_ring_size, sizeof(void *));
    core_fast_ring_use_multiple_producers(&worker->input_clean_outbound_buffer_ring);

    worker->output_message_ring_for_triage = NULL;

#if 0
    core_fast_ring_init(&worker->output_message_ring_for_triage,
                    injected_buffer_ring_size,
                    sizeof(struct thorium_message));
#endif

    core_fast_queue_init(&worker->output_message_queue_for_triage,
                    sizeof(struct thorium_message));
#endif

    thorium_scheduler_init(&worker->scheduler, thorium_node_name(worker->node),
                    worker->name);
    core_map_init(&worker->actors, sizeof(int), sizeof(int));
    core_map_iterator_init(&worker->actor_iterator, &worker->actors);

    /*
    */
    /*
    outbound_ring_capacity = 256;
    */

#ifndef THORIUM_WORKER_USE_MULTIPLE_PRODUCER_RING
    outbound_ring_capacity = THORIUM_WORKER_RING_CAPACITY;
    core_fast_ring_init(&worker->output_outbound_message_ring, outbound_ring_capacity, sizeof(struct thorium_message));
#endif

    core_fast_queue_init(&worker->output_outbound_message_queue, sizeof(struct thorium_message));

    CORE_BITMAP_CLEAR_BIT(worker->flags, FLAG_DEBUG);
    CORE_BITMAP_CLEAR_BIT(worker->flags, FLAG_BUSY);
    CORE_BITMAP_CLEAR_BIT(worker->flags, FLAG_ENABLE_ACTOR_LOAD_PROFILER);

    CORE_BITMAP_CLEAR_BIT(worker->flags, FLAG_DEBUG_ACTORS);

    if (core_command_has_argument(argc, argv, DEBUG_WORKER_OPTION)) {

#if 0
        printf("DEBUG has option %s\n", DEBUG_WORKER_OPTION);
#endif

        if (thorium_node_name(worker->node) == 0
                    && thorium_worker_name(worker) == 0) {

#if 0
            printf("DEBUG setting bit FLAG_DEBUG_ACTORS because %s\n", DEBUG_WORKER_OPTION);
#endif
            CORE_BITMAP_SET_BIT(worker->flags, FLAG_DEBUG_ACTORS);
        }
    }

    worker->epoch_used_nanoseconds = 0;
    worker->loop_used_nanoseconds = 0;
    worker->scheduling_epoch_used_nanoseconds = 0;

    worker->started_in_thread = 0;

/* 2 MiB is the default size for Linux huge pages.
 * \see https://wiki.debian.org/Hugepages
 * \see http://lwn.net/Articles/376606/
 */

    /*
     * 8 MiB
     */
    ephemeral_memory_block_size = 8388608;
    /*ephemeral_memory_block_size = 16777216;*/
    core_memory_pool_init(&worker->ephemeral_memory, ephemeral_memory_block_size,
                    MEMORY_POOL_NAME_WORKER_EPHEMERAL);

    core_memory_pool_disable_tracking(&worker->ephemeral_memory);
    core_memory_pool_enable_ephemeral_mode(&worker->ephemeral_memory);

#ifdef THORIUM_WORKER_ENABLE_LOCK
    core_lock_init(&worker->lock);
#endif

    core_set_init(&worker->evicted_actors, sizeof(int));

    core_memory_pool_init(&worker->outbound_message_memory_pool,
                    CORE_MEMORY_POOL_MESSAGE_BUFFER_BLOCK_SIZE, MEMORY_POOL_NAME_WORKER_OUTBOUND);

    /*
     * Disable the pool so that it uses allocate and free
     * directly.
     */

#ifdef CORE_MEMORY_POOL_DISABLE_MESSAGE_BUFFER_POOL
    core_memory_pool_disable(&worker->outbound_message_memory_pool);
#endif

    /*
     * Transport message buffers are fancy objects.
     */
    core_memory_pool_enable_normalization(&worker->outbound_message_memory_pool);
    core_memory_pool_enable_alignment(&worker->outbound_message_memory_pool);

    worker->ticks_without_production = 0;

    thorium_priority_assigner_init(&worker->assigner, thorium_worker_name(worker));

    /*
     * This variables should be set in
     * thorium_worker_start, but when running on 1 process with 1 thread,
     * thorium_worker_start is never called...
     */
    worker->last_report = time(NULL);
    worker->epoch_start_in_nanoseconds = core_timer_get_nanoseconds(&worker->timer);
    worker->loop_start_in_nanoseconds = worker->epoch_start_in_nanoseconds;
    worker->loop_end_in_nanoseconds = worker->loop_start_in_nanoseconds;
    worker->scheduling_epoch_start_in_nanoseconds = worker->epoch_start_in_nanoseconds;

    /*
     * Avoid valgrind warnings.
     */
    worker->epoch_load = 0;

    thorium_worker_configure_backoff(worker);

    thorium_worker_set_outbound_message_ring(worker, NULL);

    worker->last_outbound_message_block_operation = core_timer_get_nanoseconds(&worker->timer);

    thorium_multiplexer_policy_init(&worker->multiplexer_policy);
    thorium_message_multiplexer_init(&worker->multiplexer, node,
                    &worker->multiplexer_policy);

    /*
     * Set the callback object for flushing outbound messages
     * somewhere.
     */
    thorium_message_multiplexer_set_worker(&worker->multiplexer, worker);
}

void thorium_worker_destroy(struct thorium_worker *worker)
{
    void *buffer;

    /*
    thorium_message_block_destroy(&worker->message_block);
    */

    if (CORE_BITMAP_GET_BIT(worker->flags, FLAG_ENABLE_ACTOR_LOAD_PROFILER)) {
        core_buffered_file_writer_destroy(&worker->load_profile_writer);
    }

    core_map_destroy(&worker->actor_received_messages);

    /*
    thorium_worker_print_balance(worker);
    */
    while (thorium_worker_fetch_clean_outbound_buffer(worker, &buffer)) {

        core_memory_pool_free(&worker->outbound_message_memory_pool, buffer);

#ifdef THORIUM_WORKER_DEBUG_INJECTION
        ++worker->counter_freed_outbound_buffers_from_other_workers;
#endif
    }

#ifdef THORIUM_WORKER_DEBUG_INJECTION
    printf("AFTER COLLECTION\n");
    thorium_worker_print_balance(worker);

    printf("THORIUM-> output_message_queue_for_triage has %d items\n",
                    core_fast_queue_size(&worker->output_message_queue_for_triage));
    printf("THORIUM-> output_message_ring_for_triage has %d items\n",
                    core_fast_ring_size_from_producer(worker->output_message_ring_for_triage));
    printf("THORIUM-> input_clean_outbound_buffer_ring has %d items\n",
                    core_fast_ring_size_from_producer(&worker->input_clean_outbound_buffer_ring));
#endif

    core_timer_destroy(&worker->timer);

#ifdef THORIUM_WORKER_ENABLE_LOCK
    core_lock_destroy(&worker->lock);
#endif

    core_fast_ring_destroy(&worker->input_inbound_message_ring);

    core_fast_ring_destroy(&worker->input_message_ring_for_multiplexer);
    core_fast_queue_destroy(&worker->output_outbound_message_queue_for_multiplexer);

    core_fast_queue_destroy(&worker->input_inbound_message_queue);

#ifdef THORIUM_NODE_INJECT_CLEAN_WORKER_BUFFERS
    core_fast_ring_destroy(&worker->input_clean_outbound_buffer_ring);

    /*
    core_fast_ring_destroy(&worker->output_message_ring_for_triage);
    */
    worker->output_message_ring_for_triage = NULL;

    core_fast_queue_destroy(&worker->output_message_queue_for_triage);
#endif

    thorium_scheduler_destroy(&worker->scheduler);

#ifndef THORIUM_WORKER_USE_MULTIPLE_PRODUCER_RING
    core_fast_ring_destroy(&worker->output_outbound_message_ring);
#endif

    core_fast_queue_destroy(&worker->output_outbound_message_queue);

    core_map_destroy(&worker->actors);
    core_map_iterator_destroy(&worker->actor_iterator);
    core_set_destroy(&worker->evicted_actors);

    worker->node = NULL;

    worker->name = -1;
    CORE_BITMAP_SET_BIT(worker->flags, FLAG_DEAD);

    core_memory_pool_destroy(&worker->ephemeral_memory);
    core_memory_pool_destroy(&worker->outbound_message_memory_pool);

    thorium_priority_assigner_destroy(&worker->assigner);

    thorium_message_multiplexer_destroy(&worker->multiplexer);
    thorium_multiplexer_policy_destroy(&worker->multiplexer_policy);
}

struct thorium_node *thorium_worker_node(struct thorium_worker *worker)
{
    return worker->node;
}

void thorium_worker_send(struct thorium_worker *worker, struct thorium_message *message_argument)
{
    void *buffer;
    int count;
    void *old_buffer;
    int action;
    int option_enable_multiplexer;
    struct thorium_message message_copy;
    struct thorium_message *message;

#ifdef THORIUM_WORKER_SEND_TO_LOCAL_ACTOR
    int destination;
    struct thorium_actor *destination_actor;
#endif

    /*
     * Make a copy of the message to avoid side effects.
     */
    message_copy = *message_argument;
    message = &message_copy;

    option_enable_multiplexer = YES;

    tracepoint(thorium_message, worker_send, message);

    action = thorium_message_action(message);

    /*
     * There are 4 types of routes:
     *
     * 1. actor 1, actor 1 (same actor) (technically not feasible)
     *
     * 2. actor 1, worker 1, actor 2 (same worker)
     *
     * 3. actor 1, worker 1, node 1, worker 2, actor 2 (same node)
     *
     * 4. actor 1, worker 1, node 1, node 2, worker 2, actor 2 (different node)
     */

    old_buffer = thorium_message_buffer(message);

    count = thorium_message_count(message);

    /*
     * Allocate a buffer if the actor provided a NULL buffer or if it
     * provided its own buffer.
     */
    if (old_buffer == NULL
                    || old_buffer != worker->zero_copy_buffer) {

        /* use slab allocator */
        buffer = thorium_worker_allocate(worker, count);

        /* according to
         * http://stackoverflow.com/questions/3751797/can-i-call-core_memory_copy-and-core_memory_move-with-number-of-bytes-set-to-zero
         * memcpy works with a count of 0, but the addresses must be valid
         * nonetheless
         *
         * Copy the message data.
         */
        if (count > 0) {

#ifdef DISPLAY_COPY_WARNING
            printf("thorium_worker: Warning, not using zero-copy path, action %x count %d source %d destination %d\n",
                            thorium_message_action(message), count, thorium_message_source(message),
                            thorium_message_destination(message));
#endif
            core_memory_copy(buffer, old_buffer, count);
        }

        thorium_message_set_buffer(message, buffer);
    }

    /*
     * Always write metadata for any actor message.
     */
    /*
     * Set nodes for the message.
     */
    thorium_node_resolve(worker->node, message);
    thorium_message_add_metadata_to_count(message);
    thorium_message_write_metadata(message);

#ifdef THORIUM_WORKER_DEBUG_INJECTION
    ++worker->counter_allocated_outbound_buffers;
#endif

#ifdef THORIUM_WORKER_DEBUG_MEMORY
    printf("ALLOCATE %p\n", buffer);
#endif

#ifdef THORIUM_WORKER_DEBUG
    printf("[thorium_worker_send] allocated %i bytes (%i + %i) for buffer %p\n",
                    all, count, metadata_size, buffer);

    printf("thorium_worker_send old buffer: %p\n",
                    thorium_message_buffer(message));
#endif

#ifdef THORIUM_BUG_594
    if (thorium_message_action(&copy) == 30202) {
        printf("DEBUG-594 thorium_worker_send\n");
        thorium_message_print(&copy);
    }
#endif

#ifdef THORIUM_WORKER_DEBUG_20140601
    if (thorium_message_action(message) == 1100) {
        printf("DEBUG thorium_worker_send 1100\n");
    }
#endif

    /*
     * Clear the zero-copy buffer now so that
     * the next message is not falsely a zero-copy buffer.
     */
    worker->zero_copy_buffer = NULL;

#ifdef THORIUM_WORKER_SEND_TO_LOCAL_ACTOR
    /*
     * If the destination actor is on the same worker,
     * handle that directly here to avoid waiting for the node.
     */
    destination = thorium_message_destination(message);

    if (action != ACTION_ENABLE_AUTO_SCALING
                    && action != ACTION_DISABLE_AUTO_SCALING
                    && thorium_worker_has_actor(worker, destination)) {

        destination_actor = thorium_node_get_actor_from_name(worker->node, destination);

        /*
         * Remove metadata from the count.
         */
        thorium_message_remove_metadata_from_count(message);

        if (destination_actor != NULL
                 && thorium_actor_enqueue_mailbox_message(destination_actor, message)) {

            tracepoint(thorium_message, worker_send_mailbox, message);

            thorium_worker_schedule_actor(worker, destination_actor);

            tracepoint(thorium_message, worker_send_schedule, message);

            return;
        }

        /*
         * Failed to enqueue the message, add the metadata again.
         */
        thorium_message_add_metadata_to_count(message);
    }
#endif

    /*
     * Small messages use a different delivery path in which batching is performed.
     *
     * Batching is also called aggregation or multiplexing.
     */
    if (option_enable_multiplexer && count <= THORIUM_MULTIPLEXER_BUFFER_SIZE_FOR_SMALL_MESSAGES) {

        /*
         * Before enqueuing the outbound message for multiplexing,
         * 3 tasks must be performed:
         *
         * - its metadata must be written into its buffer;
         * - the count must be increased to include the metadata;
         * - the message must be resolved.
         *
         * Usually, the metadata is written inside thorium_worker_send since any
         * actor message will be sent from this function in the first place.
         */

            /*
        */

        if (thorium_message_destination_node(message) == thorium_message_source_node(message)) {

             thorium_worker_execute_local_delivery(worker, message);

        } else {

            /*
             * Queue the message for multiplexing.
             */
            core_fast_queue_enqueue(&worker->output_outbound_message_queue_for_multiplexer, message);
        }
    } else {
    /*
    thorium_worker_enqueue_message(worker, message);
    */
        /*
         * The message will use the fast path for delivery
         * because it is large enough.
         */
        core_fast_queue_enqueue(&worker->output_outbound_message_queue, message);
    }
}

void thorium_worker_start(struct thorium_worker *worker, int processor)
{
    core_thread_init(&worker->thread, thorium_worker_main, worker);

    core_thread_set_affinity(&worker->thread, processor);

    worker->started_in_thread = 1;

    core_thread_start(&worker->thread);
}

void *thorium_worker_main(void *worker1)
{
    struct thorium_worker *worker;
    int credits;
    int starting_credits;

    starting_credits = 1024;
    credits = starting_credits;

    worker = (struct thorium_worker*)worker1;

#ifdef THORIUM_WORKER_DEBUG
    thorium_worker_display(worker);
    printf("Starting worker thread\n");
#endif

    while (credits--) {

        worker->last_elapsed_nanoseconds = 0;

        CORE_DEBUGGER_JITTER_DETECTION_START(worker_main_loop);

        tracepoint(thorium_worker, main, worker);

        thorium_worker_run(worker);

        CORE_DEBUGGER_JITTER_DETECTION_END(worker_main_loop, worker->last_elapsed_nanoseconds);

        ++worker->tick_count;

        if (credits == 0) {
            if (!CORE_BITMAP_GET_BIT(worker->flags, FLAG_DEAD))
                credits = starting_credits;
        }
    }

#if 0
    printf("WORKER_THREAD EXIT\n");
#endif

    return NULL;
}

void thorium_worker_display(struct thorium_worker *worker)
{
    printf("[thorium_worker_main] node %i worker %i\n",
                    thorium_node_name(worker->node),
                    thorium_worker_name(worker));
}

int thorium_worker_name(struct thorium_worker *worker)
{
    return worker->name;
}

void thorium_worker_stop(struct thorium_worker *worker)
{

#ifdef THORIUM_WORKER_DEBUG
    thorium_worker_display(worker);
    printf("stopping worker!\n");
#endif

    /*
     * FLAG_DEAD is changed and will be read
     * by the running thread.
     *
     * Only one thread is changing this value, so no thread are needed.
     */
    CORE_BITMAP_SET_BIT(worker->flags, FLAG_DEAD);

    /* Make the change visible to other threads too
     */
    CORE_MEMORY_STORE_FENCE();

    /* Wake the worker **after** killing it.
     * So basically, there is a case where the worker is killed
     * while sleeping. But since threads are cool, the worker will
     * wake up, and die for real this time.
     */
    if (CORE_BITMAP_GET_BIT(worker->flags, FLAG_ENABLE_WAIT)) {
        /*
         * Wake up if necessary because the worker might be
         * waiting for something...
         */
        thorium_worker_signal(worker);
    }

    core_thread_join(&worker->thread);

    worker->loop_end_in_nanoseconds = core_timer_get_nanoseconds(&worker->timer);
}

int thorium_worker_is_busy(struct thorium_worker *worker)
{
    return CORE_BITMAP_GET_BIT(worker->flags, FLAG_BUSY);
}


int thorium_worker_get_scheduled_actor_count(struct thorium_worker *self)
{
    return thorium_scheduler_size(&self->scheduler);
}

int thorium_worker_get_scheduled_message_count(struct thorium_worker *worker)
{
    int value;
    struct core_map_iterator map_iterator;
    int actor_name;
    int messages;
    struct thorium_actor *actor;

    core_map_iterator_init(&map_iterator, &worker->actors);

    value = 0;
    while (core_map_iterator_get_next_key_and_value(&map_iterator, &actor_name, NULL)) {

        actor = thorium_node_get_actor_from_name(worker->node, actor_name);

        if (actor == NULL) {
            continue;
        }

        messages = thorium_actor_get_mailbox_size(actor);
        value += messages;
    }

    core_map_iterator_destroy(&map_iterator);

    return value;
}

float thorium_worker_get_epoch_load(struct thorium_worker *worker)
{
    return worker->epoch_load;
}

time_t thorium_worker_get_last_report_time(struct thorium_worker *worker)
{
    return worker->last_report;
}

float thorium_worker_get_loop_load(struct thorium_worker *worker)
{
    float loop_load;
    uint64_t elapsed_from_start;
    uint64_t current_nanoseconds;

    current_nanoseconds = worker->loop_end_in_nanoseconds;
    elapsed_from_start = current_nanoseconds - worker->loop_start_in_nanoseconds;

    /* This code path is currently not implemented when using
     * only 1 thread
     */
    if (elapsed_from_start == 0) {
        return 0;
    }

    loop_load = (0.0 + worker->loop_used_nanoseconds) / elapsed_from_start;

    /* Avoid negative zeros
     */
    if (loop_load == 0) {
        loop_load = 0;
    }

    return loop_load;
}

struct core_memory_pool *thorium_worker_get_ephemeral_memory(struct thorium_worker *worker)
{
    return &worker->ephemeral_memory;
}

/* This can only be called from the CONSUMER
 */
int thorium_worker_dequeue_actor(struct thorium_worker *worker, struct thorium_actor **actor)
{
    int value;
    int name;
    struct thorium_message message;
    struct thorium_actor *other_actor;
    int other_name;
    int operations;
    int status;
    int mailbox_size;
    int action;

    operations = THORIUM_WORKER_MAXIMUM_RECEIVED_MESSAGE_COUNT_PER_CALL;
    other_actor = NULL;

    tracepoint(thorium_worker, dequeue_message, worker);

    /*
     * Move an actor from the ring to the real actor scheduling queue
     */
    while (operations--
                    && core_fast_ring_pop_multiple_producers(&worker->input_inbound_message_ring,
                            &message)) {

        action = thorium_message_action(&message);

        CORE_DEBUGGER_ASSERT(action != ACTION_INVALID);

        /*
         * The worker always need metadata.
         */
        CORE_DEBUGGER_ASSERT(thorium_message_count(&message) > 0);

        /*
         * Give this message to the demultiplexer.
         */
        if (action == ACTION_MULTIPLEXER_MESSAGE) {
            thorium_message_multiplexer_demultiplex(&worker->multiplexer, &message);
            continue;
        }

        tracepoint(thorium_message, worker_receive, &message);

        other_name = thorium_message_destination(&message);

        other_actor = thorium_node_get_actor_from_name(worker->node, other_name);

        /*
         * The actor does not exist.
         */
        if (other_actor == NULL) {
            thorium_worker_free_message(worker, &message);

            continue;
        }

        /*
         * Remove metadata from the count.
         */
        thorium_message_remove_metadata_from_count(&message);

        /*
         * Give the message to the actor.
         *
         * If that does not work, buffer it.
         */
        if (!thorium_actor_enqueue_mailbox_message(other_actor, &message)) {
            core_fast_queue_enqueue(&worker->input_inbound_message_queue, &message);
        }

#if 0
#ifdef CORE_DEBUGGER_ENABLE_ASSERT
        if (other_actor == NULL) {
            printf("NULL pointer pulled from ring, operations %d ring size %d\n",
                            operations, core_fast_ring_size_from_consumer(&worker->input_actor_ring));
        }
#endif
#endif

        CORE_DEBUGGER_ASSERT(other_actor != NULL);

        /*
        other_name = thorium_actor_name(other_actor);
        */

#ifdef THORIUM_WORKER_DEBUG_SCHEDULER
        printf("ring.DEQUEUE %d\n", other_name);
#endif

#ifdef THORIUM_WORKER_ENABLE_EVICTION
        if (core_set_find(&worker->evicted_actors, &other_name)) {

#ifdef THORIUM_WORKER_DEBUG_SCHEDULER
            printf("ALREADY EVICTED\n");
#endif
            continue;
        }
#endif

        if (!core_map_get_value(&worker->actors, &other_name, &status)) {
            /* Add the actor to the list of actors.
             * This does nothing if it is already in the list.
             */

            status = THORIUM_SCHEDULER_STATUS_IDLE;
            core_map_add_value(&worker->actors, &other_name, &status);

            core_map_iterator_destroy(&worker->actor_iterator);
            core_map_iterator_init(&worker->actor_iterator, &worker->actors);
        }

        /* If the actor is not queued, queue it
         */
        if (status == THORIUM_SCHEDULER_STATUS_IDLE) {

            status = THORIUM_SCHEDULER_STATUS_SCHEDULED;
            core_map_update_value(&worker->actors, &other_name, &status);
            thorium_scheduler_enqueue(&worker->scheduler, other_actor);

            tracepoint(thorium_worker, scheduler_enqueue, worker);

        } else {

#ifdef THORIUM_WORKER_DEBUG_SCHEDULER
            printf("SCHEDULER %d already scheduled to run, scheduled: %d\n", other_name,
                            (int)core_set_size(&worker->queued_actors));
#endif
        }
    }

    /* Now, dequeue an actor from the real queue.
     * If it has more than 1 message, re-enqueue it
     */
    value = thorium_scheduler_dequeue(&worker->scheduler, actor);

    tracepoint(thorium_worker, scheduler_dequeue, worker);

    /* Setting name to nobody;
     * check_production at the end uses the value and the name;
     * if value is false, check_production is not using name anyway.
     */
    name = THORIUM_ACTOR_NOBODY;

    /* an actor is ready to be run and it was dequeued from the scheduling queue.
     */
    if (value) {
        name = thorium_actor_name(*actor);

#ifdef THORIUM_WORKER_DEBUG_SCHEDULER
        printf("scheduler.DEQUEUE actor %d, removed from queued actors...\n", name);
#endif

        mailbox_size = thorium_actor_get_mailbox_size(*actor);

        /* The actor has only one message and it is going to
         * be processed now.
         */
        if (mailbox_size == 1) {
#ifdef THORIUM_WORKER_DEBUG_SCHEDULER
            printf("SCHEDULER %d has no message to schedule...\n", name);
#endif
            /* Set the status of the worker to THORIUM_SCHEDULER_STATUS_IDLE
             *
             * TODO: the ring new tail might not be visible too.
             * That could possibly be a problem...
             */
            status = THORIUM_SCHEDULER_STATUS_IDLE;
            core_map_update_value(&worker->actors, &name, &status);

        /* The actor still has a lot of messages
         * to process. Keep them coming.
         */
        } else if (mailbox_size >= 2) {

            /* Add the actor to the scheduling queue if it
             * still has messages
             */

#ifdef THORIUM_WORKER_DEBUG_SCHEDULER
            printf("Scheduling actor %d again, messages: %d\n",
                        name,
                        thorium_actor_get_mailbox_size(*actor));
#endif

            /* The status is still THORIUM_SCHEDULER_STATUS_SCHEDULED
             */
            thorium_scheduler_enqueue(&worker->scheduler, *actor);


        /* The actor is scheduled to run, but the new tail is not
         * yet visible apparently.
         *
         * Solution, push back the actor in the scheduler queue, it can take a few cycles to see cache changes across cores. (MESIF protocol)
         *
         * This is done below.
         */
        } else /* if (mailbox_size == 0) */ {

            status = THORIUM_SCHEDULER_STATUS_IDLE;
            core_map_update_value(&worker->actors, &name, &status);

            value = 0;
        }

    }

#ifdef THORIUM_WORKER_ENABLE_WAIT
    thorium_worker_check_production(worker, value, name);
#endif

    return value;
}

/* This can only be called from the PRODUCER
 */
int thorium_worker_enqueue_inbound_message(struct thorium_worker *worker, struct thorium_message *message)
{
    int value;

#if 0
    CORE_DEBUGGER_ASSERT(actor != NULL);
#endif

    tracepoint(thorium_message, worker_enqueue_inbound_message, message,
                    &worker->input_inbound_message_ring);

    value = core_fast_ring_push_multiple_producers(&worker->input_inbound_message_ring, message, worker->name);

#ifdef SHOW_FULL_RING_WARNINGS
    if (!value) {
        printf("thorium_worker: Warning: ring is full, input_inbound_message_ring\n");
    }
#endif

#ifdef THORIUM_WORKER_ENABLE_WAIT
    /*
     * Do a wake up if necessary when scheduling an actor in
     * the scheduling queue.
     */
    if (value && CORE_BITMAP_GET_BIT(worker->flags, FLAG_ENABLE_WAIT)) {

        /*
         * This call checks if the thread is currently waiting.
         * If it is currently waiting, then a signal is sent
         * to tell the operating system to wake up the thread so that
         * it continues its good work for the actor computation in thorium.
         */
        thorium_worker_signal(worker);
    }
#endif

    return value;
}

int thorium_worker_enqueue_message(struct thorium_worker *worker, struct thorium_message *message)
{
    tracepoint(thorium_message, worker_enqueue_message, message);

    /* Try to push the message in the output ring
     */
    if (!thorium_worker_publish_message(worker, message)) {

#ifdef SHOW_FULL_RING_WARNINGS
        printf("ENQUEUE thorium_worker: Warning: ring is full (ring: %d queue: %d), output_outbound_message_ring\n",
                        core_fast_ring_size_from_producer(&worker->output_outbound_message_ring),
                        core_fast_queue_size(&worker->output_outbound_message_queue));
#endif
        /* If that does not work, push the message in the queue buffer.
         */
        core_fast_queue_enqueue(&worker->output_outbound_message_queue, message);

        thorium_worker_activate_backoff(worker);
    }

    return 1;
}

int thorium_worker_dequeue_message(struct thorium_worker *worker, struct thorium_message *message)
{
#ifdef THORIUM_WORKER_USE_MULTIPLE_PRODUCER_RING
    /*
     * This function is not called when using the multiple-producer ring.
     */
    return 0;
#else
    int answer;

    answer = core_fast_ring_pop_from_consumer(&worker->output_outbound_message_ring, message);

    if (answer) {
        thorium_message_set_worker(message, worker->name);

        tracepoint(thorium_message, worker_dequeue_message, message);
    }

    return answer;
#endif
}

void thorium_worker_evict_actor(struct thorium_worker *worker, int actor_name)
{
    struct thorium_actor *actor;
    int name;
    struct core_fast_queue saved_actors;

    /*
    int count;
    int value;
    */

    core_set_add(&worker->evicted_actors, &actor_name);
    core_map_delete(&worker->actors, &actor_name);
    core_fast_queue_init(&saved_actors, sizeof(struct thorium_actor *));

    /* evict the actor from the scheduling queue
     */
    while (thorium_scheduler_dequeue(&worker->scheduler, &actor)) {

        name = thorium_actor_name(actor);

        if (name != actor_name) {

            core_fast_queue_enqueue(&saved_actors,
                            &actor);
        }
    }

    while (core_fast_queue_dequeue(&saved_actors, &actor)) {
        thorium_scheduler_enqueue(&worker->scheduler, actor);
    }

    core_fast_queue_destroy(&saved_actors);

    /* Evict the actor from the ring
     */

#if 0
    count = core_fast_ring_size_from_consumer(&worker->input_actor_ring);

    while (count-- && core_fast_ring_pop_from_consumer(&worker->input_actor_ring,
                            &actor)) {

        name = thorium_actor_name(actor);

        if (name != actor_name) {

            /*
             * This can not fail logically.
             */
            value = core_fast_ring_push_from_producer(&worker->input_actor_ring,
                            &actor);

            CORE_DEBUGGER_ASSERT(value);
        }
    }
#endif

    core_map_iterator_destroy(&worker->actor_iterator);
    core_map_iterator_init(&worker->actor_iterator, &worker->actors);
}

#ifdef THORIUM_WORKER_ENABLE_LOCK
void thorium_worker_lock(struct thorium_worker *worker)
{
    core_lock_lock(&worker->lock);
}

void thorium_worker_unlock(struct thorium_worker *worker)
{
    core_lock_unlock(&worker->lock);
}
#endif

struct core_map *thorium_worker_get_actors(struct thorium_worker *worker)
{
    return &worker->actors;
}

int thorium_worker_enqueue_actor_special(struct thorium_worker *worker, struct thorium_actor *actor)
{
#if 0
    int name;

    name = thorium_actor_name(actor);

    core_set_delete(&worker->evicted_actors, &name);

    return thorium_worker_enqueue_actor(worker, actor);
#endif
    return -1;
}

int thorium_worker_get_sum_of_received_actor_messages(struct thorium_worker *worker)
{
    int value;
    struct core_map_iterator iterator;
    int actor_name;
    int messages;
    struct thorium_actor *actor;

    core_map_iterator_init(&iterator, &worker->actors);

    value = 0;
    while (core_map_iterator_get_next_key_and_value(&iterator, &actor_name, NULL)) {

        actor = thorium_node_get_actor_from_name(worker->node, actor_name);

        if (actor == NULL) {
            continue;
        }

        messages = thorium_actor_get_sum_of_received_messages(actor);

        value += messages;
    }

    core_map_iterator_destroy(&iterator);

    return value;
}

int thorium_worker_get_queued_messages(struct thorium_worker *worker)
{
    int value;
    struct core_map_iterator map_iterator;
    int actor_name;
    int messages;
    struct thorium_actor *actor;

    core_map_iterator_init(&map_iterator, &worker->actors);

    value = 0;
    while (core_map_iterator_get_next_key_and_value(&map_iterator, &actor_name, NULL)) {

        actor = thorium_node_get_actor_from_name(worker->node, actor_name);

        if (actor == NULL) {
            continue;
        }

        messages = thorium_actor_get_mailbox_size(actor);

        value += messages;
    }

    core_map_iterator_destroy(&map_iterator);

    return value;

}

float thorium_worker_get_scheduling_epoch_load(struct thorium_worker *worker)
{
    uint64_t end_time;
    uint64_t period;

    end_time = core_timer_get_nanoseconds(&worker->timer);

    period = end_time - worker->scheduling_epoch_start_in_nanoseconds;

    if (period == 0) {
        return 0;
    }

    return (0.0 + worker->scheduling_epoch_used_nanoseconds) / period;
}

void thorium_worker_reset_scheduling_epoch(struct thorium_worker *worker)
{
    worker->scheduling_epoch_start_in_nanoseconds = core_timer_get_nanoseconds(&worker->timer);

    worker->scheduling_epoch_used_nanoseconds = 0;
}

int thorium_worker_get_production(struct thorium_worker *worker, struct thorium_balancer *scheduler)
{
    struct core_map_iterator iterator;
    int name;
    struct thorium_actor *actor;
    int production;

    production = 0;
    core_map_iterator_init(&iterator, &worker->actors);

    while (core_map_iterator_get_next_key_and_value(&iterator, &name, NULL)) {

        actor = thorium_node_get_actor_from_name(worker->node, name);

        if (actor == NULL) {
            continue;
        }

        production += thorium_balancer_get_actor_production(scheduler, actor);

    }

    core_map_iterator_destroy(&iterator);

    return production;
}

int thorium_worker_get_producer_count(struct thorium_worker *worker, struct thorium_balancer *scheduler)
{
    struct core_map_iterator iterator;
    int name;
    struct thorium_actor *actor;
    int count;

    count = 0;
    core_map_iterator_init(&iterator, &worker->actors);

    while (core_map_iterator_get_next_key_and_value(&iterator, &name, NULL)) {

        actor = thorium_node_get_actor_from_name(worker->node, name);

        if (actor == NULL) {
            continue;
        }

        if (thorium_balancer_get_actor_production(scheduler, actor) > 0) {
            ++count;
        }

    }

    core_map_iterator_destroy(&iterator);
    return count;
}

/*
 *This is called from within the actor running inside this worker.
 */
void thorium_worker_free_message(struct thorium_worker *worker, struct thorium_message *message)
{
    int source_worker;
    void *buffer;
    struct thorium_worker *sibling;
    int injected_in_sibling;
    int option_use_fast_path_for_dirty_message_injection;

    option_use_fast_path_for_dirty_message_injection = YES;

    CORE_DEBUGGER_ASSERT_NOT_NULL(worker);
    CORE_DEBUGGER_ASSERT_NOT_NULL(message);

    buffer = thorium_message_buffer(message);

    /*
     * Messages that hit a worker can not have a NULL buffer.
     */
    CORE_DEBUGGER_ASSERT_NOT_NULL(buffer);

    /*
     * Nothing to do.
     */
    if (buffer == NULL) {
        return;
    }

    source_worker = thorium_message_worker(message);

    if (source_worker == worker->name) {

        /*
         * This is from the current worker
         */
        core_memory_pool_free(&worker->outbound_message_memory_pool, buffer);
#ifdef THORIUM_WORKER_DEBUG_INJECTION
        ++worker->counter_freed_outbound_buffers_from_self;
#endif

    } else {

        /*
         * This is from another fellow local worker
         * or from another BIOSAL node altogether.
         */
        CORE_DEBUGGER_ASSERT(thorium_message_buffer(message) != NULL);

        injected_in_sibling = NO;

        /*
         * Verify if the message has a marker.
         */
        if (option_use_fast_path_for_dirty_message_injection
                        && source_worker != THORIUM_WORKER_NONE) {

            CORE_DEBUGGER_ASSERT(source_worker >= 0);
            CORE_DEBUGGER_ASSERT(source_worker < worker->worker_count);

            CORE_DEBUGGER_ASSERT(thorium_message_type(message) == THORIUM_MESSAGE_TYPE_WORKER_OUTBOUND);

            /*
            printf("DEBUG_INJECT_DIRTY injecting message for local worker %d\n", source_worker);
            */

            sibling = worker->workers + source_worker;

            /*
            printf("DEBUG worker_name %d DEBUG inject\n", worker->name);
            thorium_message_print(message);
            */

            injected_in_sibling = thorium_worker_inject_clean_outbound_buffer(sibling,
                            thorium_message_buffer(message));
        }

        /*
         * Inject the message in the output ring. The message is either:
         *
         * - a node inbound message
         * or
         * - it is a worker outbound message and the input ring of this sibling worker is full.
         */
        if (!injected_in_sibling)
            thorium_worker_enqueue_message_for_triage(worker, message);
    }
}

static int thorium_worker_enqueue_message_for_triage(struct thorium_worker *worker, struct thorium_message *message)
{
#ifdef THORIUM_WORKER_DEBUG_INJECTION
    int worker_name;
#endif

    CORE_DEBUGGER_ASSERT(thorium_message_buffer(message) != NULL);

    if (!core_fast_ring_push_multiple_producers(worker->output_message_ring_for_triage, message,
                            worker->name)) {

#ifdef SHOW_FULL_RING_WARNINGS
        printf("thorium_worker: Warning: ring is full, output_message_ring_for_triage action= %x\n",
                        thorium_message_action(message));
#endif

        core_fast_queue_enqueue(&worker->output_message_queue_for_triage, message);

#ifdef THORIUM_WORKER_DEBUG_INJECTION
    } else {
        /*
         * Update software counters.
         */
        worker_name = thorium_message_worker(message);

        if (worker_name >= 0) {
            ++worker->counter_injected_outbound_buffers_other_local_workers;
        } else {
            ++worker->counter_injected_inbound_buffers_from_thorium_core;
        }
#endif
    }

    return 1;
}

#if 0
int thorium_worker_dequeue_message_for_triage(struct thorium_worker *worker, struct thorium_message *message)
{
    int value;

    value = core_fast_ring_pop_from_consumer(&worker->output_message_ring_for_triage, message);

#ifdef CORE_DEBUGGER_ENABLE_ASSERT
    if (value) {
        CORE_DEBUGGER_ASSERT(thorium_message_buffer(message) != NULL);
    }
#endif

    return value;
}
#endif

/* Just return the number of queued messages.
 */
int thorium_worker_get_message_production_score(struct thorium_worker *worker)
{
#ifdef THORIUM_WORKER_USE_MULTIPLE_PRODUCER_RING
    return -1;
#else
    int score;

    score = 0;

    score += core_fast_ring_size_from_producer(&worker->output_outbound_message_ring);

    score += core_fast_queue_size(&worker->output_outbound_message_queue);

    return score;
#endif
}

void thorium_worker_run(struct thorium_worker *worker)
{
    struct thorium_actor *actor;
    struct thorium_message other_message;
    int name;

    int worker_index;
    int destination_node;
    struct thorium_worker *worker_for_multiplexer;

#ifdef THORIUM_NODE_INJECT_CLEAN_WORKER_BUFFERS
    void *buffer;
#endif

#ifdef THORIUM_NODE_ENABLE_INSTRUMENTATION
    time_t current_time;
    int elapsed;
    int period;
    uint64_t current_nanoseconds;
    uint64_t elapsed_nanoseconds;
#endif

#ifdef THORIUM_WORKER_DEBUG
    int tag;
    int destination;
    struct thorium_message *message;
#endif

    tracepoint(thorium_worker, run_enter, worker);

#ifdef THORIUM_WORKER_ENABLE_LOCK
    thorium_worker_lock(worker);
#endif

#ifdef THORIUM_NODE_ENABLE_INSTRUMENTATION
    period = THORIUM_NODE_LOAD_PERIOD;
    current_time = time(NULL);

    elapsed = current_time - worker->last_report;

    if (elapsed >= period) {

        current_nanoseconds = core_timer_get_nanoseconds(&worker->timer);

#ifdef THORIUM_WORKER_DEBUG_LOAD
        printf("DEBUG Updating load report\n");
#endif
        elapsed_nanoseconds = current_nanoseconds - worker->epoch_start_in_nanoseconds;

        if (elapsed_nanoseconds > 0) {
            worker->epoch_load = (0.0 + worker->epoch_used_nanoseconds) / elapsed_nanoseconds;
            worker->epoch_used_nanoseconds = 0;
            worker->last_wake_up_count = core_thread_get_wake_up_count(&worker->thread);

            /* \see http://stackoverflow.com/questions/9657993/negative-zero-in-c
             */
            if (worker->epoch_load == 0) {
                worker->epoch_load = 0;
            }

            worker->epoch_start_in_nanoseconds = current_nanoseconds;
            worker->last_report = current_time;
        }

#ifdef THORIUM_WORKER_PRINT_SCHEDULING_QUEUE

        /*
        if (thorium_node_name(worker->node) == 0
                        && worker->name == 0) {
                        */

        thorium_scheduler_print(&worker->scheduler,
                        thorium_node_name(worker->node),
                        worker->name);
            /*
        }
        */
#endif

        if (CORE_BITMAP_GET_BIT(worker->flags, FLAG_DEBUG_ACTORS)) {
            thorium_worker_print_actors(worker, NULL);
        }
    }
#endif

#ifdef THORIUM_WORKER_DEBUG
    if (CORE_BITMAP_GET_BIT(worker->flags, FLAG_DEBUG)) {
        printf("DEBUG worker/%d thorium_worker_run\n",
                        thorium_worker_name(worker));
    }
#endif

    /*
     * Check for messages in inbound FIFO
     */
    if (!CORE_BITMAP_GET_BIT(worker->flags, FLAG_OUTPUT_OUTBOUND_MESSAGE_RING_IS_FULL)
                    && thorium_worker_dequeue_actor(worker, &actor)) {

#ifdef THORIUM_WORKER_DEBUG
        message = biosal_work_message(&work);
        tag = thorium_message_action(message);
        destination = thorium_message_destination(message);

        if (tag == ACTION_ASK_TO_STOP) {
            printf("DEBUG pulled ACTION_ASK_TO_STOP for %d\n",
                            destination);
        }
#endif

        /*
         * Update the priority of the actor
         * before starting the timer because this is part of the
         * runtime system (RTS).
         */

#ifdef THORIUM_UPDATE_SCHEDULING_PRIORITIES
        thorium_priority_assigner_update(&worker->scheduler, actor);
#endif

#ifdef THORIUM_NODE_ENABLE_INSTRUMENTATION
        core_timer_start(&worker->timer);
#endif

        CORE_BITMAP_SET_BIT(worker->flags, FLAG_BUSY);

        /*
         * Dispatch message to a worker
         */
        thorium_worker_work(worker, actor);

        CORE_BITMAP_CLEAR_BIT(worker->flags, FLAG_BUSY);
        CORE_BITMAP_CLEAR_BIT(worker->flags, FLAG_ENABLE_WAIT);

#ifdef THORIUM_NODE_ENABLE_INSTRUMENTATION
        core_timer_stop(&worker->timer);

        elapsed_nanoseconds = core_timer_get_elapsed_nanoseconds(&worker->timer);

        if (elapsed_nanoseconds >= THORIUM_GRANULARITY_WARNING_THRESHOLD) {
        }

        worker->epoch_used_nanoseconds += elapsed_nanoseconds;
        worker->loop_used_nanoseconds += elapsed_nanoseconds;
        worker->scheduling_epoch_used_nanoseconds += elapsed_nanoseconds;

        worker->last_elapsed_nanoseconds = elapsed_nanoseconds;
#endif
    }

    /* queue buffered message
     */
    if (core_fast_queue_dequeue(&worker->output_outbound_message_queue, &other_message)) {

        if (!thorium_worker_publish_message(worker, &other_message)) {

#ifdef SHOW_FULL_RING_WARNINGS
            printf("thorium_worker: Warning: ring is full => output_outbound_message_ring\n");
#endif

            core_fast_queue_enqueue(&worker->output_outbound_message_queue, &other_message);
        }
    }

#ifdef THORIUM_NODE_INJECT_CLEAN_WORKER_BUFFERS
    /*
     * Free outbound buffers, if any
     */

    if (thorium_worker_fetch_clean_outbound_buffer(worker, &buffer)) {
        core_memory_pool_free(&worker->outbound_message_memory_pool, buffer);

#ifdef THORIUM_WORKER_DEBUG_INJECTION
        ++worker->counter_freed_outbound_buffers_from_other_workers;
#endif
    }
#endif

    /*
     * Transfer messages for triage
     */
    if (core_fast_queue_dequeue(&worker->output_message_queue_for_triage, &other_message)) {

        CORE_DEBUGGER_ASSERT(thorium_message_buffer(&other_message) != NULL);
        thorium_worker_free_message(worker, &other_message);
    }

    /*
     * Transfer message to actor
     */
    if (core_fast_queue_dequeue(&worker->input_inbound_message_queue, &other_message)) {

        name = thorium_message_destination(&other_message);
        actor = thorium_node_get_actor_from_name(worker->node, name);

        if (!thorium_actor_enqueue_mailbox_message(actor, &other_message)) {
            core_fast_queue_enqueue(&worker->input_inbound_message_queue, &other_message);
        } else {

            /*
             * Schedule it too.
             */
            thorium_worker_schedule_actor(worker, actor);
        }
    }

    /*
     * Flush outbound message block.
     */
    /*
    thorium_worker_flush_outbound_message_block(worker);
    */

#ifdef THORIUM_WORKER_ENABLE_LOCK
    thorium_worker_unlock(worker);
#endif

    thorium_worker_do_backoff(worker);

    /*
     * Transfer messages to multiplex. This process will be done
     * by another worker.
     */
    if (core_fast_queue_dequeue(&worker->output_outbound_message_queue_for_multiplexer,
        &other_message)) {

        destination_node = thorium_message_destination_node(&other_message);

        worker_index = destination_node % worker->worker_count;
        /*
        worker_index = 0;
        */

        worker_for_multiplexer = worker->workers + worker_index;

        if (!thorium_worker_enqueue_message_for_multiplexer(worker_for_multiplexer,
                                &other_message)) {

            /*
             * Put it back in the queue if the ring is full already.
             */
            core_fast_queue_enqueue(&worker->output_outbound_message_queue_for_multiplexer,
                            &other_message);
        }
    }

    /*
     * Consume messages for multiplexing.
     */
    if (core_fast_ring_pop_multiple_producers(&worker->input_message_ring_for_multiplexer,
                            &other_message)) {

        /*
         * Multiplex the message.
         */
        if (thorium_message_multiplexer_multiplex(&worker->multiplexer,
                                &other_message)) {
            /*
             * The message must be recycled now.
             * It will be sent to its destination.
             */

            thorium_worker_free_message(worker, &other_message);

        } else {
            /*
             * Otherwise, this is a regular outbound message.
             */
            if (!core_fast_ring_push_multiple_producers(worker->output_outbound_message_ring_multiple,
                                &other_message, worker->name)) {

                /*
                 * Buffer the message locally if the outbound ring is full.
                 */
                core_fast_queue_enqueue(&worker->output_outbound_message_queue, &other_message);
            }
        }
    }

    thorium_message_multiplexer_test(&worker->multiplexer);

    tracepoint(thorium_worker, run_exit, worker);
}

static void thorium_worker_work(struct thorium_worker *worker, struct thorium_actor *actor)
{
    int dead;
    int actor_name;
    int status;

#ifdef THORIUM_WORKER_DEBUG
    int tag;
    int destination;
#endif

    tracepoint(thorium_worker, work_enter, worker);

    actor_name = thorium_actor_name(actor);

#ifdef THORIUM_WORKER_DEBUG_SCHEDULER
    printf("WORK actor %d\n", actor_name);
#endif

    /* the actor died while the work was queued.
     */
    if (thorium_actor_dead(actor)) {

        printf("NOTICE actor is dead already (thorium_worker_work)\n");

        return;
    }

#ifdef THORIUM_DISABLE_LOCKLESS_ACTORS
    /* lock the actor to prevent another worker from making work
     * on the same actor at the same time
     */
    if (thorium_actor_trylock(actor) != CORE_LOCK_SUCCESS) {

        printf("Warning: CONTENTION worker %d could not lock actor %d, returning the message...\n",
                        thorium_worker_name(worker),
                        actor_name);

        return;
    }
#endif

    /* the actor died while this worker was waiting for the lock
     */
    if (thorium_actor_dead(actor)) {

        printf("DEBUG thorium_worker_work actor died while the worker was waiting for the lock.\n");
#ifdef THORIUM_WORKER_DEBUG
#endif
        /*thorium_actor_unlock(actor);*/
        return;
    }

    /* call the actor receive code
     */
    thorium_actor_set_worker(actor, worker);

    thorium_actor_work(actor);

    /* Free ephemeral memory
     */
    core_memory_pool_free_all(&worker->ephemeral_memory);

    dead = thorium_actor_dead(actor);

    if (dead) {

        core_map_delete(&worker->actors, &actor_name);

        if (CORE_BITMAP_GET_BIT(worker->flags,
                                FLAG_ENABLE_ACTOR_LOAD_PROFILER)) {
            thorium_actor_write_profile(actor, &worker->load_profile_writer);
        }
        thorium_node_notify_death(worker->node, actor);
    }

    thorium_actor_set_worker(actor, NULL);

#ifdef THORIUM_WORKER_DEBUG_20140601
    if (CORE_BITMAP_GET_BIT(worker->flags, FLAG_DEBUG)) {
        printf("DEBUG worker/%d after dead call\n",
                        thorium_worker_name(worker));
    }
#endif

    /*
     * If the actor still has messages, schedule it again.
     */
    core_map_get_value(&worker->actors, &actor_name, &status);
    if (status == THORIUM_SCHEDULER_STATUS_IDLE
                    && thorium_actor_get_mailbox_size(actor) > 0) {

        status = THORIUM_SCHEDULER_STATUS_SCHEDULED;
        core_map_update_value(&worker->actors, &actor_name, &status);

        thorium_scheduler_enqueue(&worker->scheduler, actor);
    }

#ifdef THORIUM_DISABLE_LOCKLESS_ACTORS
    /* Unlock the actor.
     * This does not do anything if a death notification
     * was sent to the node
     */
    thorium_actor_unlock(actor);
#endif

#ifdef THORIUM_WORKER_DEBUG
    printf("thorium_worker_work Freeing buffer %p %i tag %i\n",
                    buffer, thorium_message_count(message),
                    thorium_message_action(message));
#endif

#ifdef THORIUM_WORKER_DEBUG_20140601
    if (CORE_BITMAP_GET_BIT(worker->flags, FLAG_DEBUG)) {
        printf("DEBUG worker/%d exiting thorium_worker_work\n",
                        thorium_worker_name(worker));
    }
#endif

    tracepoint(thorium_worker, work_exit, worker);
}

void thorium_worker_wait(struct thorium_worker *worker)
{

    if (!worker->started_in_thread) {
        return;
    }

    core_thread_wait(&worker->thread);
}

void thorium_worker_signal(struct thorium_worker *worker)
{
    if (!worker->started_in_thread) {
        return;
    }

    core_thread_signal(&worker->thread);
}

uint64_t thorium_worker_get_epoch_wake_up_count(struct thorium_worker *worker)
{
    return core_thread_get_wake_up_count(&worker->thread) - worker->last_wake_up_count;
}

uint64_t thorium_worker_get_loop_wake_up_count(struct thorium_worker *worker)
{
    return core_thread_get_wake_up_count(&worker->thread);
}

void thorium_worker_enable_waiting(struct thorium_worker *self)
{
    CORE_BITMAP_SET_BIT(self->flags, FLAG_ENABLE_WAIT);
}

void thorium_worker_check_production(struct thorium_worker *worker, int value, int name)
{
    uint64_t time;
    uint64_t elapsed;
    struct thorium_actor *other_actor;
    int mailbox_size;
    int status;
    uint64_t threshold;

    /*
     * If no actor is scheduled to run, things are getting out of hand
     * and this is bad for business.
     *
     * So, here, an actor is poked for inactivity
     */
    if (!value) {
        ++worker->ticks_without_production;
    } else {
        worker->ticks_without_production = 0;
    }

    /*
     * If too many cycles were spent doing nothing,
     * check the fast ring since there could be issue in the
     * cache coherency of the CPU, even with the memory fences.
     *
     * This should not happen theoretically.
     *
     */
    if (worker->ticks_without_production >= THORIUM_WORKER_UNPRODUCTIVE_TICK_LIMIT) {

        if (core_map_iterator_get_next_key_and_value(&worker->actor_iterator, &name, NULL)) {

            other_actor = thorium_node_get_actor_from_name(worker->node, name);

            mailbox_size = 0;
            if (other_actor != NULL) {
                mailbox_size = thorium_actor_get_mailbox_size(other_actor);
            }

            if (mailbox_size > 0) {
                thorium_scheduler_enqueue(&worker->scheduler, other_actor);

                status = THORIUM_SCHEDULER_STATUS_SCHEDULED;
                core_map_update_value(&worker->actors, &name, &status);
            }
        } else {

            /* Rewind the iterator.
             */
            core_map_iterator_destroy(&worker->actor_iterator);
            core_map_iterator_init(&worker->actor_iterator, &worker->actors);

            /*worker->ticks_without_production = 0;*/
        }

    /*
     * If there is still nothing, tell the operating system that the thread
     * needs to sleep.
     *
     * The operating system is:
     * - Linux on Cray XE6,
     * - Linux on Cray XC30,
     * - IBM Compute Node Kernel (CNK) on IBM Blue Gene/Q),
     */
        if (CORE_BITMAP_GET_BIT(worker->flags, FLAG_ENABLE_WAIT)) {

            /* This is a first warning
             */
            if (worker->waiting_start_time == 0) {
                worker->waiting_start_time = core_timer_get_nanoseconds(&worker->timer);

            } else {

                time = core_timer_get_nanoseconds(&worker->timer);

                elapsed = time - worker->waiting_start_time;

                threshold = THORIUM_WORKER_UNPRODUCTIVE_MICROSECONDS_FOR_WAIT;

                /* Convert microseconds to nanoseconds
                 */
                threshold *= 1000;

                /* Verify the elapsed time.
                 * There are 1000 nanoseconds in 1 microsecond.
                 */
                if (elapsed >= threshold) {
                    /*
                     * Here, the worker will wait until it receives a signal.
                     * Such a signal will mean that something is ready to be consumed.
                     */

                    /* Reset the time
                     */
                    worker->waiting_start_time = 0;

#ifdef THORIUM_WORKER_DEBUG_WAIT_SIGNAL
                    printf("DEBUG worker/%d will wait, elapsed %d\n",
                                    worker->name, (int)elapsed);
#endif

                    /*
                     */
                    thorium_worker_wait(worker);
                }
            }
        }
    }
}

int thorium_worker_inject_clean_outbound_buffer(struct thorium_worker *self, void *buffer)
{
    int value;

    value = core_fast_ring_push_multiple_producers(&self->input_clean_outbound_buffer_ring,
                    &buffer, self->name);

#ifdef SHOW_FULL_RING_WARNINGS
    if (!value) {
        printf("thorium_worker: Warning: ring is full, input_clean_outbound_buffer_ring\n");
    }
#endif

    return value;
}

int thorium_worker_fetch_clean_outbound_buffer(struct thorium_worker *self, void **buffer)
{
    return core_fast_ring_pop_multiple_producers(&self->input_clean_outbound_buffer_ring,
                    buffer);
}

void thorium_worker_print_balance(struct thorium_worker *self)
{
#ifdef THORIUM_WORKER_DEBUG_INJECTION
    int balance;

    balance = 0;
    balance += self->counter_allocated_outbound_buffers;
    balance -= self->counter_freed_outbound_buffers_from_other_workers;
    balance -= self->counter_freed_outbound_buffers_from_self;

    printf("THORIUM worker/%d \n"
                    "    counter_allocated_outbound_buffers %d\n"
                    "    counter_freed_outbound_buffers_from_self %d\n"
                    "    counter_freed_outbound_buffers_from_other_workers %d\n"
                    "    balance %d\n"
                    "    counter_injected_outbound_buffers_other_local_workers %d\n"
                    "    counter_injected_inbound_buffers_from_thorium_core %d\n",
                    self->name,
                    self->counter_allocated_outbound_buffers, self->counter_freed_outbound_buffers_from_self,
                    self->counter_freed_outbound_buffers_from_other_workers,
                    balance,
                    self->counter_injected_outbound_buffers_other_local_workers,
                    self->counter_injected_inbound_buffers_from_thorium_core);
#endif
}

void thorium_worker_examine(struct thorium_worker *self)
{
    printf("DEBUG_WORKER Name= %d\n", self->name);

    core_memory_pool_examine(&self->ephemeral_memory);
    core_memory_pool_examine(&self->outbound_message_memory_pool);

#ifndef THORIUM_WORKER_USE_MULTIPLE_PRODUCER_RING
    printf("RING (producer)= output_outbound_message_ring size= %d\n",
                    core_fast_ring_size_from_producer(&self->output_outbound_message_ring));
#endif
    printf("QUEUE= output_outbound_message_queue size= %d\n",
                    core_fast_queue_size(&self->output_outbound_message_queue));

    printf("RING (consumer)= input_inbound_message_ring size= %d\n",
                    core_fast_ring_size_from_producer(&self->input_inbound_message_ring));

    printf("RING (consumer)= input_clean_outbound_buffer_ring size= %d\n",
                    core_fast_ring_size_from_producer(&self->input_clean_outbound_buffer_ring));

    printf("RING (producer)= output_message_ring_for_triage size= %d\n",
                    core_fast_ring_size_from_producer(self->output_message_ring_for_triage));
    printf("QUEUE= output_message_queue_for_triage size= %d\n",
                    core_fast_queue_size(&self->output_message_queue_for_triage));
}

int thorium_worker_spawn(struct thorium_worker *self, int script)
{
    int name;

    name = thorium_node_spawn(self->node, script);

#if 0
    if (CORE_BITMAP_GET_BIT(node->flags, FLAG_ENABLE_ACTOR_LOAD_PROFILER)) {
        thorium_actor_enable_profiler(actor);
    }
#endif

    return name;
}

void thorium_worker_enable_profiler(struct thorium_worker *self)
{
    char file_name[100];

    CORE_BITMAP_SET_BIT(self->flags, FLAG_ENABLE_ACTOR_LOAD_PROFILER);

    sprintf(file_name, "node_%d_worker_%d_actor_load_profile.txt", thorium_node_name(self->node),
                    self->name);

    core_buffered_file_writer_init(&self->load_profile_writer, file_name);

    core_buffered_file_writer_printf(&self->load_profile_writer, THORIUM_ACTOR_PROFILER_HEADER);
}

void *thorium_worker_allocate(struct thorium_worker *self, size_t count)
{
    void *buffer;
    int all;
    int metadata_size;

    /*
     * Make some room for the metadata.
     */
    metadata_size = THORIUM_MESSAGE_METADATA_SIZE;
    all = count + metadata_size;

    /* use slab allocator to allocate buffer... */
    buffer = (char *)core_memory_pool_allocate(&self->outbound_message_memory_pool,
                    all * sizeof(char));

    self->zero_copy_buffer = buffer;

    return buffer;
}

static void thorium_worker_configure_backoff(struct thorium_worker *self)
{
#ifdef THORIUM_WORKER_CONFIG_USE_BACKOFF
    CORE_BITMAP_CLEAR_BIT(self->flags, FLAG_OUTPUT_OUTBOUND_MESSAGE_RING_IS_FULL);
#endif
}

static void thorium_worker_activate_backoff(struct thorium_worker *self)
{
#ifdef THORIUM_WORKER_CONFIG_USE_BACKOFF
    /*
     * Switch mode if stalled to avoid over-production
     */
    if (!CORE_BITMAP_GET_BIT(self->flags, FLAG_OUTPUT_OUTBOUND_MESSAGE_RING_IS_FULL)) {

        CORE_BITMAP_SET_BIT(self->flags, FLAG_OUTPUT_OUTBOUND_MESSAGE_RING_IS_FULL);

#ifdef DEBUG_BACKOFF
        printf("thorium_worker_activate_backoff !\n");
#endif
    }
#endif
}

static void thorium_worker_do_backoff(struct thorium_worker *self)
{
#ifdef THORIUM_WORKER_CONFIG_USE_BACKOFF
    /*
     * The idea of backoff for output outbound message rings is presented
     * here.
     * The thorium_actor_receive() function of an actor may generate a lot of messages.
     * This function is called in thorium_actor_work(), which is called in
     * thorium_worker_work.
     *
     * To avoid producer-consumer imbalance, the backoff idea basicaly waits
     * for the ring to empty before doing anything else.
     */
    int size;
    int capacity;
    int threshold;

    /*
     * Wait for the ring to clean up if necessary.
     */
    if (CORE_BITMAP_GET_BIT(self->flags, FLAG_OUTPUT_OUTBOUND_MESSAGE_RING_IS_FULL)) {

#ifdef THORIUM_WORKER_USE_MULTIPLE_PRODUCER_RING
        /*
         * In the case of a multiple-producer outbound message ring,
         * we instead observe the size of the private output outbound
         * message queue.
         */
        size = core_fast_queue_size(&self->output_outbound_message_queue);
        capacity = 512;
#else
        size = core_fast_ring_size_from_producer(&self->output_outbound_message_ring);
        capacity = core_fast_ring_capacity(&self->output_outbound_message_ring);
#endif

        threshold = capacity / 2;

        /*
         * Clear the bit if messages were consumed by the Thorium node pacing thread.
         */
        if (size <= threshold) {
            /*
             * At this point, the ring is no longer full !
             */
            CORE_BITMAP_CLEAR_BIT(self->flags, FLAG_OUTPUT_OUTBOUND_MESSAGE_RING_IS_FULL);
        }
    }
#endif
}

int thorium_worker_get_input_message_ring_size(struct thorium_worker *self)
{
    return core_fast_ring_size_from_producer(&self->input_inbound_message_ring);
}

static void thorium_worker_schedule_actor(struct thorium_worker *self, struct thorium_actor *actor)
{
    int status;
    int name;
    int *bucket;

    name = thorium_actor_name(actor);
    bucket = core_map_get(&self->actors, &name);

    status = THORIUM_SCHEDULER_STATUS_SCHEDULED;

    /*
     * Already scheduled.
     */
    if (*bucket == status)
        return;

    *bucket = status;
    thorium_scheduler_enqueue(&self->scheduler, actor);
}

static int thorium_worker_has_actor(struct thorium_worker *self, int actor)
{
    return core_map_get(&self->actors, &actor) != NULL;
}

void thorium_worker_set_triage_message_ring(struct thorium_worker *self, struct core_fast_ring *ring)
{
    self->output_message_ring_for_triage = ring;
}

void thorium_worker_set_outbound_message_ring(struct thorium_worker *self, struct core_fast_ring *ring)
{
#ifdef THORIUM_WORKER_USE_MULTIPLE_PRODUCER_RING
    self->output_outbound_message_ring_multiple = ring;
#endif
}

static int thorium_worker_publish_message(struct thorium_worker *self, struct thorium_message *message)
{
    tracepoint(thorium_worker, publish_message, self->name, self->tick_count,
                    thorium_message_count(message),
                    0,
                    0, self->last_outbound_message_block_operation);

    return core_fast_ring_push_multiple_producers(self->output_outbound_message_ring_multiple,
                       message, self->name);

#if 0
    int return_value;

    return_value = 0;

    /*
     * The block is not full, so the message can be appended.
     */
    if (!thorium_message_block_full(&self->message_block)) {
        thorium_message_block_add_message(&self->message_block, message);
        return_value = 1;

        /*
         * This is an instrumentation probe for the
         * tracepoint event "thorium_message:worker_publish_message".
         */
        tracepoint(thorium_message, worker_publish_message, message,
                    self->output_outbound_message_ring_multiple);
    }

#ifdef THORIUM_WORKER_USE_MULTIPLE_PRODUCER_RING

    thorium_worker_flush_outbound_message_block(self);

    /*
     * Was the message given in argument consumed at all ?
     */
    return return_value;

#else
    return core_fast_ring_push_from_producer(&self->output_outbound_message_ring, message);
#endif
#endif
}

#if 0
void thorium_worker_flush_outbound_message_block(struct thorium_worker *self)
{
    int push;
    uint64_t current_time;
    int group_messages;

    if (thorium_message_block_empty(&self->message_block))
        return;

    group_messages = 1;
    push = 1;

    current_time = core_timer_get_nanoseconds(&self->timer);

    /*
     * Check if the block must be flushed.
     */
    if (group_messages
                    && !thorium_message_block_full(&self->message_block))
        push = 0;

    /*
     * Check some sort of timeout.
     */
    if (!push) {

        if ((current_time - self->last_outbound_message_block_operation) >= (200 * 1000))
            push = 1;
    }

    if (push) {
        if (core_fast_ring_push_multiple_producers(self->output_outbound_message_ring_multiple,
                        &self->message_block, self->name)) {

            tracepoint(thorium_worker, flush_outbound_message_block, self->name, self->tick_count,
                    thorium_message_block_size(&self->message_block),
                    core_fast_ring_size_from_producer(self->output_outbound_message_ring_multiple));

            thorium_message_block_clear(&self->message_block);
            self->last_outbound_message_block_operation = core_timer_get_nanoseconds(&self->timer);
        }
    }
}
#endif

static int thorium_worker_enqueue_message_for_multiplexer(struct thorium_worker *self,
                struct thorium_message *message)
{
    return core_fast_ring_push_multiple_producers(&self->input_message_ring_for_multiplexer,
                    message, self->name);
}

void thorium_worker_set_siblings(struct thorium_worker *self,
                struct thorium_worker *workers, int worker_count)
{
    /*
     * Fetch some numbers for the node
     */
    self->workers = workers;
    self->worker_count = worker_count;
}

int thorium_worker_enqueue_outbound_message(struct thorium_worker *self,
                struct thorium_message *message)
{
    return core_fast_queue_enqueue(&self->output_outbound_message_queue,
                    message);
}

struct core_memory_pool *thorium_worker_get_outbound_message_memory_pool(struct thorium_worker *self)
{
    return &self->outbound_message_memory_pool;
}

void thorium_worker_execute_local_delivery(struct thorium_worker *self, struct thorium_message *message)
{
    int message_was_pushed;
    int worker_index;
    struct thorium_worker *destination_worker;
    int destination;
    struct thorium_actor *destination_actor;
    int option_use_fast_delivery;

    option_use_fast_delivery = YES;

    /*
     * If this is a message for a local actor, send it right away
     * to the destination actor.
     */
    destination = thorium_message_destination(message);
    destination_actor = thorium_node_get_actor_from_name(self->node, destination);
    message_was_pushed = 0;

    if (option_use_fast_delivery && destination_actor != NULL) {

        worker_index = thorium_actor_assigned_worker(destination_actor);

        if (worker_index != THORIUM_WORKER_NONE) {
            destination_worker = self->workers + worker_index;

            if (thorium_worker_enqueue_inbound_message(destination_worker, message))
                message_was_pushed = 1;
        }
    }

    /*
     * Use the regular route if the fast path code path failed.
     */
    if (!message_was_pushed)
        core_fast_queue_enqueue(&self->output_outbound_message_queue,
                    message);
}

int thorium_worker_get_random_number(struct thorium_worker *self)
{
    return rand_r(&self->random_seed);
}
