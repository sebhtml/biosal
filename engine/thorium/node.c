
/* Must be defined before any header is included.
 * Used for setting affinity.
 *
 * \see http://lists.mcs.anl.gov/pipermail/petsc-dev/2012-May/008453.html
 */

#include "node.h"

#include "worker_buffer.h"

#include "tracepoints/tracepoints.h"

#include <core/structures/vector.h>
#include <core/structures/map_iterator.h>
#include <core/structures/vector_iterator.h>
#include <core/structures/set_iterator.h>

#include <core/helpers/vector_helper.h>
#include <core/helpers/bitmap.h>

#include <core/system/command.h>
#include <core/system/memory.h>
#include <core/system/tracer.h>

#include <core/system/debugger.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>

#include <inttypes.h>

#if 1
#undef CORE_DEBUGGER_JITTER_DETECTION_START
#undef CORE_DEBUGGER_JITTER_DETECTION_END
#define CORE_DEBUGGER_JITTER_DETECTION_START(name)
#define CORE_DEBUGGER_JITTER_DETECTION_END(name, actor_time)
#endif

#define MEMORY_POOL_NAME_NODE_INBOUND      0xee1344f0
#define MEMORY_POOL_NAME_NODE_OUTBOUND     0xf3ad5880
#define MEMORY_POOL_NAME_NONE              0xcef49361
#define MEMORY_POOL_NAME_ACTORS            0x37ddf367

/* options */
/*
#define THORIUM_NODE_DEBUG_ACTOR_COUNTERS
*/
#define THORIUM_NODE_REUSE_DEAD_INDICES

/*
 * Debug node regulator engine.
 */
/*
#define DEBUG_NODE_REGULATOR
*/

/*
#define USE_RANDOM_ACTOR_NAMES
*/

/* debugging options */
/*
#define THORIUM_NODE_DEBUG

#define THORIUM_NODE_DEBUG_RECEIVE_SYSTEM
*/
/*
#define THORIUM_NODE_DEBUG_RUN "Yes"
*/

/*
#define THORIUM_NODE_SIMPLE_INITIAL_ACTOR_NAMES
#define THORIUM_NODE_DEBUG_SPAWN

#define THORIUM_NODE_DEBUG_ACTOR_COUNTERS
#define THORIUM_NODE_DEBUG_SUPERVISOR
#define THORIUM_NODE_DEBUG_LOOP

#define THORIUM_NODE_DEBUG_SPAWN_KILL
*/

/*
#define TRANSPORT_DEBUG_ISSUE_594
*/

#define FLAG_PRINT_LOAD                 0
#define FLAG_DEBUG                      1
#define FLAG_PRINT_STRUCTURE            2
#define FLAG_STARTED                    3
#define FLAG_PRINT_COUNTERS             4
#define FLAG_USE_TRANSPORT              5
#define FLAG_SEND_IN_THREAD             6
#define FLAG_WORKER_IN_MAIN_THREAD      7
#define FLAG_WORKERS_IN_THREADS         8
#define FLAG_EXAMINE                    9
#define FLAG_ENABLE_ACTOR_LOAD_PROFILES 10
#define FLAG_MULTIPLEXER_IS_DISABLED    11
#define FLAG_REGULATOR_IS_ENABLED       12
#define FLAG_PROFILE_MESSAGE_TRANSPORT  13

/*
 * Enable the regulator.
 */
/*
#define THORIUM_NODE_CONFIG_USE_REGULATOR
*/

struct thorium_node *thorium_node_global_self;

void thorium_node_handle_signal(int signal);
void thorium_node_register_signal_handlers(struct thorium_node *self);

void thorium_node_print_structure(struct thorium_node *self, struct thorium_actor *actor);
int thorium_node_has_actor(struct thorium_node *self, int name);

struct thorium_worker_pool *thorium_node_get_worker_pool(struct thorium_node *self);

void thorium_node_toggle_debug_mode(struct thorium_node *self);

void thorium_node_reset_actor_counters(struct thorium_node *self);

void thorium_node_test_requests(struct thorium_node *self);

void thorium_node_free_worker_buffer(struct thorium_node *self,
                struct thorium_worker_buffer *worker_buffer);

void thorium_node_send_to_actor(struct thorium_node *self, int name, struct thorium_message *message);
void thorium_node_check_efficiency(struct thorium_node *self);
int thorium_node_send_system(struct thorium_node *self, struct thorium_message *message);

void thorium_node_do_message_triage(struct thorium_node *self);
void thorium_node_recycle_message(struct thorium_node *self, struct thorium_message *message);

void thorium_node_resolve(struct thorium_node *self, struct thorium_message *message);
/*
 * Generate an actor name.
 */
int thorium_node_generate_random_name(struct thorium_node *self,
                int minimal_value, int maximum_value);

void thorium_node_inject_outbound_buffer(struct thorium_node *self, struct thorium_worker_buffer *worker_buffer);

void thorium_node_send_to_node_empty(struct thorium_node *self, int destination, int tag);
int thorium_node_receive_system(struct thorium_node *self, struct thorium_message *message);
void thorium_node_set_initial_actor(struct thorium_node *self, int node_name, int actor);
int thorium_node_allocate_actor_index(struct thorium_node *self);

void thorium_node_inject_message_in_worker_pool(struct thorium_node *self, struct thorium_message *message);

void *thorium_node_main(void *node1);
int thorium_node_running(struct thorium_node *self);
void thorium_node_start_send_thread(struct thorium_node *self);
int thorium_node_generate_name(struct thorium_node *self);

int thorium_node_actor_node(struct thorium_node *self, int name);
int thorium_node_actor_index(struct thorium_node *self, int name);
void thorium_node_run_loop(struct thorium_node *self);

/*
 * Private prototypes for inbound message regulator
 */
void thorium_node_regulator_configure(struct thorium_node *self);
void thorium_node_regulator_run(struct thorium_node *self);
int thorium_node_regulator_must_wait(struct thorium_node *self);

int thorium_node_has_script(struct thorium_node *self, struct thorium_script *script);
void thorium_node_send_messages(struct thorium_node *self);

void thorium_node_init(struct thorium_node *node, int *argc, char ***argv)
{
    int i;
    int workers;
    int threads;
    char *required_threads;
    int detected;
    int actor_capacity;
    int processor;

    node->tick = 0;

#ifdef THORIUM_NODE_USE_CUSTOM_TRACEPOINTS
    /*
     * Tracepoint session
     */
    thorium_tracepoint_session_init(node->name, node->tick);
    thorium_tracepoint_session_add_tracepoint(node->name, node->tick,
                    THORIUM_TRACEPOINT_node_run_loop_print, "node:run_loop_print");
    thorium_tracepoint_session_add_tracepoint(node->name, node->tick,
                    THORIUM_TRACEPOINT_node_run_loop_receive, "node:run_loop_receive");
    thorium_tracepoint_session_add_tracepoint(node->name, node->tick,
                    THORIUM_TRACEPOINT_node_run_loop_run, "node:run_loop_run");
    thorium_tracepoint_session_add_tracepoint(node->name, node->tick,
                    THORIUM_TRACEPOINT_node_run_loop_send, "node:run_loop_send");
    thorium_tracepoint_session_add_tracepoint(node->name, node->tick,
                    THORIUM_TRACEPOINT_node_run_loop_pool_work, "node:run_loop_pool_work");
    thorium_tracepoint_session_add_tracepoint(node->name, node->tick,
                    THORIUM_TRACEPOINT_node_run_loop_test_requests, "node:run_loop_test_requests");
    thorium_tracepoint_session_add_tracepoint(node->name, node->tick,
                    THORIUM_TRACEPOINT_node_run_loop_do_triage, "node:run_loop_do_triage");
#endif

    core_timer_init(&node->timer);

#ifdef THORIUM_NODE_USE_TICKS
    node->tick_count = 0;
#endif

#ifdef THORIUM_NODE_DEBUG_INJECTION
    node->counter_allocated_node_inbound_buffers = 0;
    node->counter_freed_multiplexed_inbound_buffers = 0;
    node->counter_allocated_node_outbound_buffers = 0;
    node->counter_freed_thorium_outbound_buffers = 0;
    node->counter_freed_injected_node_inbound_buffers = 0;
    node->counter_injected_buffers_for_local_workers = 0;
    node->counter_injected_transport_outbound_buffer_for_workers = 0;
#endif

    CORE_BITMAP_CLEAR(node->flags);
    CORE_BITMAP_CLEAR_BIT(node->flags, FLAG_PROFILE_MESSAGE_TRANSPORT);
    node->worker_for_triage = 0;

    /*
    printf("Booting node\n");
    */
    /* register signal handlers
     */
    thorium_node_register_signal_handlers(node);

    core_set_init(&node->auto_scaling_actors, sizeof(int));

    CORE_BITMAP_CLEAR_BIT(node->flags, FLAG_STARTED);
    CORE_BITMAP_CLEAR_BIT(node->flags, FLAG_PRINT_LOAD);
    CORE_BITMAP_CLEAR_BIT(node->flags, FLAG_PRINT_STRUCTURE);
    CORE_BITMAP_CLEAR_BIT(node->flags, FLAG_DEBUG);
    CORE_BITMAP_CLEAR_BIT(node->flags, FLAG_PRINT_COUNTERS);
    CORE_BITMAP_CLEAR_BIT(node->flags, FLAG_EXAMINE);
    CORE_BITMAP_CLEAR_BIT(node->flags, FLAG_ENABLE_ACTOR_LOAD_PROFILES);
    CORE_BITMAP_CLEAR_BIT(node->flags, FLAG_MULTIPLEXER_IS_DISABLED);

    thorium_node_global_self = node;

    node->start_time = time(NULL);
    node->last_transport_event_time = node->start_time;
    node->last_report_time = 0;
    node->last_auto_scaling = node->start_time;

#ifdef THORIUM_NODE_USE_DETERMINISTIC_ACTOR_NAMES
    node->current_actor_name = THORIUM_ACTOR_NOBODY;
#endif

    /*
     * Build memory pools
     */

    core_memory_pool_init(&node->actor_memory_pool, 2097152, MEMORY_POOL_NAME_ACTORS);
    core_memory_pool_disable(&node->actor_memory_pool);

    core_memory_pool_init(&node->inbound_message_memory_pool,
                    CORE_MEMORY_POOL_MESSAGE_BUFFER_BLOCK_SIZE, MEMORY_POOL_NAME_NODE_INBOUND);
    core_memory_pool_enable_normalization(&node->inbound_message_memory_pool);
    core_memory_pool_enable_alignment(&node->inbound_message_memory_pool);

#ifdef CORE_MEMORY_POOL_DISABLE_MESSAGE_BUFFER_POOL
    core_memory_pool_disable(&node->inbound_message_memory_pool);
#endif

    core_memory_pool_init(&node->outbound_message_memory_pool,
                    CORE_MEMORY_POOL_MESSAGE_BUFFER_BLOCK_SIZE, MEMORY_POOL_NAME_NODE_OUTBOUND);

    core_memory_pool_enable_normalization(&node->outbound_message_memory_pool);
    core_memory_pool_enable_alignment(&node->outbound_message_memory_pool);

#ifdef CORE_MEMORY_POOL_DISABLE_MESSAGE_BUFFER_POOL
    core_memory_pool_disable(&node->outbound_message_memory_pool);
#endif

    thorium_transport_init(&node->transport, node, argc, argv,
                    &node->inbound_message_memory_pool,
                    &node->outbound_message_memory_pool);

    node->provided = thorium_transport_get_provided(&node->transport);
    node->name = thorium_transport_get_rank(&node->transport);
    node->nodes = thorium_transport_get_size(&node->transport);

    /*
     * On Blue Gene/Q, the starting time and PID is the same for all ranks.
     * Therefore, the name of the rank is also used.
     */
    srand(node->start_time * getpid() * node->name);

#ifdef THORIUM_NODE_INJECT_CLEAN_WORKER_BUFFERS
    core_fast_queue_init(&node->output_clean_outbound_buffer_queue, sizeof(struct thorium_worker_buffer));
#endif

    CORE_BITMAP_SET_BIT(node->flags, FLAG_USE_TRANSPORT);

    CORE_BITMAP_CLEAR_BIT(node->flags, FLAG_DEBUG);

#ifdef THORIUM_NODE_DEBUG_LOOP
    CORE_BITMAP_SET_BIT(node->flags, FLAG_DEBUG);
#endif

    core_map_init(&node->scripts, sizeof(int), sizeof(struct thorium_script *));

    /* the rank number is needed to decide on
     * the number of threads
     */

    /* the default is 1 thread */
    threads = 1;

    node->threads = threads;
    node->argc = *argc;
    node->argv = *argv;

    if (core_command_has_argument(node->argc, node->argv, "-debug-memory-pools"))
        CORE_BITMAP_SET_BIT(node->flags, FLAG_EXAMINE);

    if (core_command_has_argument(node->argc, node->argv, "-print-counters")) {
        CORE_BITMAP_SET_BIT(node->flags, FLAG_PRINT_COUNTERS);
    }

    if (core_command_has_argument(node->argc, node->argv, "-print-load")) {
        CORE_BITMAP_SET_BIT(node->flags, FLAG_PRINT_LOAD);
    }

    if (core_command_has_argument(node->argc, node->argv, "-print-structure")) {
        CORE_BITMAP_SET_BIT(node->flags, FLAG_PRINT_STRUCTURE);
    }

    for (i = 0; i < *argc; i++) {
        if (strcmp((*argv)[i], "-threads-per-node") == 0 && i + 1 < *argc) {
            /*printf("thorium_node_init threads: %s\n",
                            (*argv)[i + 1]);*/

            required_threads = (*argv)[i + 1];

            /* -threads-per-node all
             *
             * \see http://stackoverflow.com/questions/4586405/get-number-of-cpus-in-linux-using-c
             */
#ifdef _SC_NPROCESSORS_ONLN
            if (strcmp(required_threads, "all") == 0) {
                node->threads = sysconf(_SC_NPROCESSORS_ONLN);
#ifdef THORIUM_NODE_DEBUG
                printf("DEBUG using all threads: %d\n", node->threads);
#endif
                continue;
            }
#endif

            /* -threads-per-node 5,6,9
             */
            detected = thorium_node_threads_from_string(node, required_threads, node->name);

            if (detected > 0) {
                node->threads = detected;

#ifdef THORIUM_NODE_DEBUG
                printf("DEBUG %s node %d : %d threads\n", required_threads,
                                node_name, detected);
#endif

                continue;
            }

            /* -threads-per-node 99
             */
            node->threads = atoi(required_threads);

#ifdef THORIUM_NODE_DEBUG
            printf("DEBUG using %d threads\n", node->threads);
#endif
        }
    }

    if (node->threads < 1) {
        node->threads = 1;
    }

    CORE_BITMAP_SET_BIT(node->flags, FLAG_WORKER_IN_MAIN_THREAD);

    if (node->threads >= 2) {
        CORE_BITMAP_CLEAR_BIT(node->flags, FLAG_WORKER_IN_MAIN_THREAD);
    }

    CORE_BITMAP_CLEAR_BIT(node->flags, FLAG_WORKERS_IN_THREADS);

    /* with 2 threads, one of them runs a worker */
    if (node->threads >= 2) {
        CORE_BITMAP_SET_BIT(node->flags, FLAG_WORKERS_IN_THREADS);
    }

	/*
     * 3 cases with T threads using transport_init:
     *
     * Case 0: T is 1, ask for THORIUM_THREAD_SINGLE
     * Design: receive, run, and send in main thread
     *
     * Case 1: if T is 2, ask for THORIUM_THREAD_FUNNELED
     * Design: receive and send in main thread, workers in (T-1) thread
     *
     * Case 2: if T is 3 or more, ask for THORIUM_THREAD_MULTIPLE
     *
     * Design: if THORIUM_THREAD_MULTIPLE is provided, receive in main thread, send in 1 thread,
     * workers in (T - 2) threads, otherwise delegate the case to Case 1
     */
    workers = 1;

    if (node->threads == 1) {
        workers = node->threads - 0;

    } else if (node->threads == 2) {
        workers = node->threads - 1;

    } else if (node->threads >= 3) {

        /* the number of workers depends on whether or not
         * THORIUM_THREAD_MULTIPLE is provided
         */
    }

    CORE_BITMAP_CLEAR_BIT(node->flags, FLAG_SEND_IN_THREAD);

    if (node->threads >= 3) {

#ifdef THORIUM_NODE_DEBUG
        printf("DEBUG= threads: %i\n", node->threads);
#endif
        if (node->provided == THORIUM_THREAD_MULTIPLE) {
            CORE_BITMAP_SET_BIT(node->flags, FLAG_SEND_IN_THREAD);
            workers = node->threads - 2;

        /* assume THORIUM_THREAD_FUNNELED
         */
        } else {

#ifdef THORIUM_NODE_DEBUG
            printf("DEBUG= THORIUM_THREAD_MULTIPLE was not provided...\n");
#endif
            workers = node->threads - 1;
        }
    }

#ifdef THORIUM_NODE_DEBUG
    printf("DEBUG threads: %i workers: %i send_in_thread: %i\n",
                    node->threads, workers,
                    CORE_BITMAP_GET_BIT(node->flags, FLAG_SEND_IN_THREAD));
#endif

    thorium_worker_pool_init(&node->worker_pool, workers, node);

    actor_capacity = 131072;
    node->dead_actors = 0;
    node->alive_actors = 0;

    core_vector_init(&node->actors, sizeof(struct thorium_actor));

    /* it is necessary to reserve because work units will point
     * to actors so their addresses can not be changed
     */
    core_vector_reserve(&node->actors, actor_capacity);

    /*
     * Multiply by 2 to avoid resizing
     */
    core_map_init_with_capacity(&node->actor_names, sizeof(int), sizeof(int), actor_capacity * 2);

    core_vector_init(&node->initial_actors, sizeof(int));

    /*printf("BEFORE\n");*/
    core_vector_resize(&node->initial_actors, thorium_node_nodes(node));
    /*printf("AFTER\n");*/

    node->received_initial_actors = 0;
    node->ready = 0;

    core_lock_init(&node->spawn_and_death_lock);
    core_lock_init(&node->script_lock);
    core_lock_init(&node->auto_scaling_lock);

    core_queue_init(&node->dead_indices, sizeof(int));

#ifdef THORIUM_NODE_USE_COUNTERS
    core_counter_init(&node->counter);
#endif

    processor = workers;

    if (node->nodes != 1) {
        processor = -1;
    }

    core_set_affinity(processor);

    if (CORE_BITMAP_GET_BIT(node->flags, FLAG_PRINT_LOAD)) {
        printf("thorium_node: booted node %d (%d nodes), threads: %d, workers: %d, pacing: %d\n",
                    node->name,
            node->nodes,
            node->threads,
            thorium_worker_pool_worker_count(&node->worker_pool),
            1);

#if 0
        thorium_transport_print(&node->transport);
#endif
    }

    thorium_multiplexer_policy_init(&node->multiplexer_policy);
    thorium_message_multiplexer_init(&node->multiplexer, node,
                    &node->multiplexer_policy);

    if (thorium_message_multiplexer_is_disabled(&node->multiplexer)) {
        CORE_BITMAP_SET_BIT(node->flags, FLAG_MULTIPLEXER_IS_DISABLED);
    }

    if (core_command_has_argument(node->argc, node->argv, "-enable-actor-load-profiler")) {
        thorium_worker_pool_enable_profiler(&node->worker_pool);
        CORE_BITMAP_SET_BIT(node->flags, FLAG_ENABLE_ACTOR_LOAD_PROFILES);
    }

    node->worker_count = thorium_worker_pool_worker_count(&node->worker_pool);

    thorium_node_regulator_configure(node);

    thorium_transport_profiler_init(&node->transport_profiler);

    if (core_command_has_argument(node->argc, node->argv, "-enable-transport-profiler")) {

        printf("Enable transport profiler\n");

        CORE_BITMAP_SET_BIT(node->flags, FLAG_PROFILE_MESSAGE_TRANSPORT);
    }
}

void thorium_node_destroy(struct thorium_node *node)
{
#ifdef THORIUM_NODE_DEBUG_INJECTION
    int active_requests;
#endif

#ifdef THORIUM_NODE_USE_CUSTOM_TRACEPOINTS
    thorium_tracepoint_session_destroy(node->name, node->tick);
#endif

    /*
     * Print the report if requested.
     */
    if (CORE_BITMAP_GET_BIT(node->flags, FLAG_PROFILE_MESSAGE_TRANSPORT)) {
        thorium_transport_profiler_print_report(&node->transport_profiler);
    }

    thorium_transport_profiler_destroy(&node->transport_profiler);

    core_timer_destroy(&node->timer);

    if (CORE_BITMAP_GET_BIT(node->flags, FLAG_EXAMINE))
        thorium_node_examine(node);

#ifdef THORIUM_NODE_DEBUG_INJECTION
    active_requests = thorium_transport_get_active_request_count(&node->transport);

    printf("THORIUM DEBUG output_clean_outbound_buffer_queue has %d items\n",
          core_fast_queue_size(&node->output_clean_outbound_buffer_queue));

    printf("node/%d \n"
             "   counter_allocated_node_inbound_buffers %d\n"
             "   counter_freed_injected_node_inbound_buffers %d\n"
             "   counter_freed_thorium_outbound_buffers %d\n"
             "   counter_injected_buffers_for_local_workers %d\n"
             "   counter_injected_transport_outbound_buffer_for_workers %d\n"
             "   active_requests %d\n",
             node->name,
             node->counter_allocated_node_inbound_buffers,
             node->counter_freed_injected_node_inbound_buffers,
             node->counter_freed_thorium_outbound_buffers,
             node->counter_injected_buffers_for_local_workers,
             node->counter_injected_transport_outbound_buffer_for_workers,
             active_requests);
#endif

    core_set_destroy(&node->auto_scaling_actors);

    core_map_destroy(&node->actor_names);

    /*printf("BEFORE DESTROY\n");*/
    core_vector_destroy(&node->initial_actors);
    /*printf("AFTER DESTROY\n");*/

    core_map_destroy(&node->scripts);
    core_lock_destroy(&node->spawn_and_death_lock);
    core_lock_destroy(&node->auto_scaling_lock);
    core_lock_destroy(&node->script_lock);

    core_vector_destroy(&node->actors);

    core_queue_destroy(&node->dead_indices);

    thorium_worker_pool_destroy(&node->worker_pool);

    thorium_transport_destroy(&node->transport);
    thorium_message_multiplexer_destroy(&node->multiplexer);
    thorium_multiplexer_policy_destroy(&node->multiplexer_policy);

#ifdef THORIUM_NODE_USE_COUNTERS
    core_counter_destroy(&node->counter);
#endif

    core_fast_queue_destroy(&node->output_clean_outbound_buffer_queue);

    /*
     * Destroy the memory pool after the rest.
     */
    core_memory_pool_destroy(&node->actor_memory_pool);
    core_memory_pool_destroy(&node->inbound_message_memory_pool);
    core_memory_pool_destroy(&node->outbound_message_memory_pool);

}

int thorium_node_threads_from_string(struct thorium_node *node,
                char *required_threads, int index)
{
    int j;
    int commas = 0;
    int start;
    int value;
    int length;

    start = 0;
    length = strlen(required_threads);

    /* first, count commas to change the index
     */
    for (j = 0; j < length; j++) {
        if (required_threads[j] == ',') {
            commas++;
        }
    }

    index %= (commas + 1);

    commas = 0;

    for (j = 0; j < length; j++) {
        if (required_threads[j] == ',') {
            if (index == commas) {
                required_threads[j] = '\0';
                value = atoi(required_threads + start);
                required_threads[j] = ',';
                return value;
            }
            commas++;
            start = j + 1;
        }
    }

    if (commas == 0) {
        return -1;
    }

    /* the last integer is not followed
     * by a comma
     */
    if (index == commas) {
        value = atoi(required_threads + start);
        return value;
    } else {
        /* recursive call...
         * this never happens because the index is changed at the beginning
         * of the function.
         */
        return thorium_node_threads_from_string(node, required_threads, index - commas -1);
    }

    return -1;
}

void thorium_node_set_supervisor(struct thorium_node *node, int name, int supervisor)
{
    struct thorium_actor *actor;

    if (name == THORIUM_ACTOR_NOBODY) {
        return;
    }

#ifdef THORIUM_NODE_DEBUG_SUPERVISOR
    printf("DEBUG thorium_node_set_supervisor %d %d\n", name, supervisor);
#endif

    actor = thorium_node_get_actor_from_name(node, name);

#ifdef THORIUM_NODE_DEBUG_SUPERVISOR
    printf("DEBUG set supervisor %d %d %p\n", name, supervisor, (void *)actor);
#endif

    thorium_actor_set_supervisor(actor, supervisor);
}

int thorium_node_actors(struct thorium_node *node)
{
    return core_vector_size(&node->actors);
}

int thorium_node_spawn(struct thorium_node *node, int script)
{
    struct thorium_script *script1;
    int size;
    void *state;
    int name;
#ifdef THORIUM_ACTOR_GATHER_MESSAGE_METADATA
    struct thorium_actor *actor;
#endif

    /* in the current implementation, there can only be one initial
     * actor on each node
     */
    if (CORE_BITMAP_GET_BIT(node->flags, FLAG_STARTED) == 0
                    && thorium_node_actors(node) > 0) {
        return -1;
    }

    script1 = thorium_node_find_script(node, script);

    if (script1 == NULL) {
        return -1;
    }

    /*
     * Get the number of bytes required to create such an actor.
     */
    size = thorium_script_size(script1);

    /*
     * Lock the node for:
     * allocating and initializing the actor (and
     * also generating and registering a name.
     */
    core_lock_lock(&node->spawn_and_death_lock);

    state = core_memory_pool_allocate(&node->actor_memory_pool, size);
    name = thorium_node_spawn_state(node, state, script1);

    /*
     * Make the name visible to other cores.
     */
    core_memory_store_fence();

    /* send the initial actor to the master node
     */
    if (thorium_node_actors(node) == 1) {
        struct thorium_message message;
        thorium_message_init(&message, ACTION_THORIUM_NODE_ADD_INITIAL_ACTOR, sizeof(name), &name);
        thorium_node_send_to_node(node, 0, &message);

#ifdef THORIUM_ACTOR_GATHER_MESSAGE_METADATA
        /* initial actors are their own spawners.
         */
        actor = thorium_node_get_actor_from_name(node, name);

        core_counter_add(thorium_actor_counter(actor), CORE_COUNTER_SPAWNED_ACTORS, 1);
#endif
    }

#ifdef THORIUM_NODE_USE_COUNTERS
    core_counter_add(&node->counter, CORE_COUNTER_SPAWNED_ACTORS, 1);
#endif

#ifdef THORIUM_NODE_DEBUG_SPAWN_KILL
    printf("DEBUG node/%d thorium_node_spawn actor/%d script/%x\n",
                    thorium_node_name(node),
                    name, script);
#endif

    core_lock_unlock(&node->spawn_and_death_lock);

    return name;
}

int thorium_node_spawn_state(struct thorium_node *node, void *state,
                struct thorium_script *script)
{
    struct thorium_actor *actor;
    int name;
    int *bucket;
    int index;

    /* can not spawn any more actor
     */
    if (core_vector_size(&node->actors) == core_vector_capacity(&node->actors)) {
        printf("Error: can not spawn more actors, capacity was reached...\n");
        return THORIUM_ACTOR_NOBODY;
    }

#ifdef THORIUM_NODE_DEBUG
    printf("DEBUG thorium_node_spawn_state\n");
#endif

    /* add an actor in the vector of actors
     */
    index = thorium_node_allocate_actor_index(node);
    actor = (struct thorium_actor *)core_vector_at(&node->actors, index);

    /* actors have random names to enforce
     * the acquaintance paradigm
     */
    name = thorium_node_generate_name(node);

    thorium_actor_init(actor, state, script, name, node);

    if (CORE_BITMAP_GET_BIT(node->flags, FLAG_ENABLE_ACTOR_LOAD_PROFILES)) {
        thorium_actor_enable_profiler(actor);
    }

    /* register the actor name
     */
    bucket = core_map_add(&node->actor_names, &name);
    *bucket = index;

    /*
     * Make the name visible to all threads
     */

    core_memory_store_fence();

#ifdef THORIUM_NODE_DEBUG_SPAWN
    printf("DEBUG added Actor %d, index is %d, bucket: %p\n", name, index,
                    (void *)bucket);
#endif

    node->alive_actors++;

    return name;
}

int thorium_node_allocate_actor_index(struct thorium_node *node)
{
    int index;

#ifdef THORIUM_NODE_REUSE_DEAD_INDICES
    if (core_queue_dequeue(&node->dead_indices, &index)) {

#ifdef THORIUM_NODE_DEBUG_SPAWN
        printf("DEBUG node/%d thorium_node_allocate_actor_index using an old index %d, size %d\n",
                        thorium_node_name(node),
                        index, core_vector_size(&node->actors));
#endif

        return index;
    }
#endif

    index = (int)core_vector_size(&node->actors);
    core_vector_resize(&node->actors, core_vector_size(&node->actors) + 1);

    return index;
}

int thorium_node_generate_name(struct thorium_node *node)
{
    int minimal_value;
    int maximum_value;
    int name;
    struct thorium_actor *actor;
    int node_name;
    int difference;
    int nodes;

#ifdef THORIUM_NODE_DEBUG
    printf("DEBUG thorium_node_generate_name\n");
#endif

#ifdef THORIUM_NODE_SIMPLE_INITIAL_ACTOR_NAMES
    if (thorium_node_actors(node) == 0) {
        return thorium_node_name(node);
    }
#endif

    node_name = thorium_node_name(node);
    actor = NULL;
    name = THORIUM_ACTOR_NOBODY;

    /* reserve  the first numbers
     */
    minimal_value = 4 * thorium_node_nodes(node) + thorium_node_name(node) + 1000000;
    maximum_value = 2000000000;

#ifdef THORIUM_NODE_DEBUG
    printf("DEBUG assigning name\n");
#endif

    nodes = thorium_node_nodes(node);

    /*
     * Find a name that is not already used.
     */
    while (actor != NULL || name < 0) {
        name = thorium_node_generate_random_name(node, minimal_value, maximum_value);

        difference = node_name - name % node->nodes;
        /* add the difference */
        name += difference;

        /* disallow names between 0 and nodes - 1
         */
        if (name < nodes) {
            continue;
        }

        actor = thorium_node_get_actor_from_name(node, name);

        /*printf("DEBUG trying %d... %p\n", name, (void *)actor);*/
    }

#ifdef THORIUM_NODE_DEBUG_SPAWN
    printf("DEBUG node %d assigned name %d\n", node->name, name);
#endif

    /*return node->name + node->nodes * thorium_node_actors(node);*/

    return name;
}

void thorium_node_set_initial_actor(struct thorium_node *node, int node_name, int actor)
{
    int *bucket;

    bucket = core_vector_at(&node->initial_actors, node_name);
    *bucket = actor;

#ifdef THORIUM_NODE_DEBUG_INITIAL_ACTORS
    printf("DEBUG thorium_node_set_initial_actor node %d actor %d, %d actors\n",
                    node_name, actor, core_vector_size(&node->initial_actors));
#endif
}

int thorium_node_run(struct thorium_node *node)
{
    float load;
    int print_final_load;
    int i;

    if (CORE_BITMAP_GET_BIT(node->flags, FLAG_PRINT_COUNTERS)) {

        printf("----------------------------------------------\n");
        printf("biosal> node/%d: %d threads, %d workers\n", thorium_node_name(node),
                    thorium_node_thread_count(node),
                    thorium_node_worker_count(node));
    }

    CORE_BITMAP_SET_BIT(node->flags, FLAG_STARTED);

    if (CORE_BITMAP_GET_BIT(node->flags, FLAG_WORKERS_IN_THREADS)) {
#ifdef THORIUM_NODE_DEBUG_RUN
        printf("THORIUM_NODE_DEBUG_RUN DEBUG starting %i worker threads\n",
                        thorium_worker_pool_worker_count(&node->worker_pool));
#endif
        thorium_worker_pool_start(&node->worker_pool);
    }


    if (CORE_BITMAP_GET_BIT(node->flags, FLAG_SEND_IN_THREAD)) {
#ifdef THORIUM_NODE_DEBUG_RUN
        printf("THORIUM_NODE_DEBUG_RUN starting send thread\n");
#endif
        thorium_node_start_send_thread(node);
    }

#ifdef THORIUM_NODE_DEBUG_RUN
    printf("THORIUM_NODE_DEBUG_RUN entering loop in thorium_node_run\n");
#endif

    thorium_node_run_loop(node);

#ifdef THORIUM_NODE_DEBUG_RUN
    printf("THORIUM_NODE_DEBUG_RUN after loop in thorium_node_run\n");
#endif

    if (CORE_BITMAP_GET_BIT(node->flags, FLAG_WORKERS_IN_THREADS)) {
        thorium_worker_pool_stop(&node->worker_pool);
    }

    if (CORE_BITMAP_GET_BIT(node->flags, FLAG_PRINT_LOAD)) {
        thorium_worker_pool_print_load(&node->worker_pool, THORIUM_WORKER_POOL_LOAD_EPOCH);
        thorium_worker_pool_print_load(&node->worker_pool, THORIUM_WORKER_POOL_LOAD_LOOP);
    }

    if (CORE_BITMAP_GET_BIT(node->flags,
                            FLAG_SEND_IN_THREAD)) {
        core_thread_join(&node->thread);
    }

    /* Always print counters at the end, this is useful.
     */
    if (CORE_BITMAP_GET_BIT(node->flags, FLAG_PRINT_COUNTERS)) {
#ifdef THORIUM_NODE_USE_COUNTERS
        thorium_node_print_counters(node);
#else
        printf("thorium_node: please recompile with THORIUM_NODE_USE_COUNTERS\n");
#endif
    }

    /*
     * Print global load for this node...
     *
     * The automated tests rely on this "COMPUTATION LOAD" line to decide whether or
     * not each test passes or fails.
     */

    print_final_load = 1;

    for (i = 0; i < node->argc; i++) {
        if (strstr(node->argv[i], "-help") != NULL
                        || strstr(node->argv[i], "-version") != NULL) {
            print_final_load = 0;
        }
    }
    if (CORE_BITMAP_GET_BIT(node->flags, FLAG_PRINT_LOAD)
                    || print_final_load) {

        load = thorium_worker_pool_get_computation_load(&node->worker_pool);

        printf("thorium_node: node/%d COMPUTATION LOAD %.2f\n",
                    thorium_node_name(node),
                    load);
    }

    return 0;
}

void thorium_node_start_initial_actor(struct thorium_node *node)
{
    int actors;
    int bytes;
    void *buffer;
    struct thorium_actor *actor;
    int i;
    int name;
    struct thorium_message message;

    actors = core_vector_size(&node->actors);

    bytes = core_vector_pack_size(&node->initial_actors);

#ifdef THORIUM_NODE_DEBUG_INITIAL_ACTORS
    printf("DEBUG packing %d initial actors\n",
                    core_vector_size(&node->initial_actors));
#endif

    buffer = core_memory_pool_allocate(&node->inbound_message_memory_pool, bytes);
    core_vector_pack(&node->initial_actors, buffer);

    for (i = 0; i < actors; ++i) {
        actor = (struct thorium_actor *)core_vector_at(&node->actors, i);
        name = thorium_actor_name(actor);

        /* initial actors are supervised by themselves... */
        thorium_actor_set_supervisor(actor, name);

        thorium_message_init(&message, ACTION_START, bytes, buffer);

        thorium_node_send_to_actor(node, name, &message);

        thorium_message_destroy(&message);
    }

    /*
    core_memory_pool_free(&node->inbound_message_memory_pool, buffer);
    */
}

int thorium_node_running(struct thorium_node *node)
{
    time_t current_time;
    int elapsed;
    int active_requests;

    /* wait until all actors are dead... */
    if (node->alive_actors > 0) {

#ifdef THORIUM_NODE_DEBUG_RUN
        printf("THORIUM_NODE_DEBUG_RUN alive workers > 0\n");
#endif
        return 1;
    }

    /*
     * wait for the requests to complete.
     */
    active_requests = thorium_transport_get_active_request_count(&node->transport);
    if (active_requests > 0) {

            /*
        printf("ACTIVE %d\n", active_requests);
        */

        return 1;
    }

    current_time = time(NULL);

    elapsed = current_time - node->last_transport_event_time;

    /*
     * Shut down if there is no actor and
     * if no message were pulled for a duration of at least
     * 4 seconds.
     */
    if (CORE_BITMAP_GET_BIT(node->flags, FLAG_USE_TRANSPORT)
                    && elapsed < 4) {
#ifdef THORIUM_NODE_DEBUG_RUN
        printf("THORIUM_NODE_DEBUG_RUN a message was received in the last period %d %d\n",
                        (int)current_time, (int)node->last_transport_event_time);
#endif
        return 1;
    }

    return 0;
}

/* TODO, this needs THORIUM_THREAD_MULTIPLE, this has not been tested */
void thorium_node_start_send_thread(struct thorium_node *node)
{
    core_thread_init(&node->thread, thorium_node_main, node);
    core_thread_start(&node->thread);
}

void *thorium_node_main(void *node1)
{
    struct thorium_node *node;

    node = (struct thorium_node*)node1;

    while (thorium_node_running(node)) {
        thorium_node_send_messages(node);
    }

    return NULL;
}

int thorium_node_receive_system(struct thorium_node *node, struct thorium_message *message)
{
    int tag;
    void *buffer;
    int name;
    int bytes;
    int i;
    int nodes;
    int source;
    struct thorium_message new_message;

#ifdef THORIUM_NODE_DEBUG_RECEIVE_SYSTEM
    int count;

    printf("DEBUG thorium_node_receive_system\n");
#endif

    tag = thorium_message_action(message);

    if (tag == ACTION_THORIUM_NODE_ADD_INITIAL_ACTOR) {

#ifdef THORIUM_NODE_DEBUG_RECEIVE_SYSTEM
        printf("DEBUG ACTION_THORIUM_NODE_ADD_INITIAL_ACTOR received\n");
#endif

        source = thorium_message_source(message);
        buffer = thorium_message_buffer(message);
        name = *(int *)buffer;
        thorium_node_set_initial_actor(node, source, name);

        nodes = thorium_node_nodes(node);
        node->received_initial_actors++;

        if (node->received_initial_actors == nodes) {

#ifdef THORIUM_NODE_DEBUG_RECEIVE_SYSTEM
            printf("DEBUG send ACTION_THORIUM_NODE_ADD_INITIAL_ACTOR to all nodes\n");
#endif

            bytes = core_vector_pack_size(&node->initial_actors);
            buffer = core_memory_pool_allocate(&node->outbound_message_memory_pool, bytes);
            core_vector_pack(&node->initial_actors, buffer);

            thorium_message_init(&new_message, ACTION_THORIUM_NODE_ADD_INITIAL_ACTORS, bytes, buffer);

            for (i = 0; i < nodes; i++) {
                thorium_node_send_to_node(node, i, &new_message);
            }

#if 0
            /*
             * thorium_node_send_to_node does a copy.
             */
            core_memory_pool_free(&node->outbound_message_memory_pool, buffer);
#endif
        }

        return 1;

    } else if (tag == ACTION_THORIUM_NODE_ADD_INITIAL_ACTORS) {

#ifdef THORIUM_NODE_DEBUG_RECEIVE_SYSTEM
        printf("DEBUG ACTION_THORIUM_NODE_ADD_INITIAL_ACTORS received\n");
#endif

        buffer = thorium_message_buffer(message);
        source = thorium_message_source_node(message);

        core_vector_destroy(&node->initial_actors);
        core_vector_init(&node->initial_actors, 0);
        core_vector_unpack(&node->initial_actors, buffer);

#ifdef THORIUM_NODE_DEBUG_RECEIVE_SYSTEM
        count = thorium_message_count(message);
        printf("DEBUG buffer size: %d, unpacked %d actor names from node/%d\n",
                        count, core_vector_size(&node->initial_actors),
                        source);
#endif

        thorium_node_send_to_node_empty(node, source, ACTION_THORIUM_NODE_ADD_INITIAL_ACTORS_REPLY);

        return 1;

    } else if (tag == ACTION_THORIUM_NODE_ADD_INITIAL_ACTORS_REPLY) {

        node->ready++;
        nodes = thorium_node_nodes(node);
        source = thorium_message_source_node(message);

#ifdef THORIUM_NODE_DEBUG_RECEIVE_SYSTEM
        printf("node/%d DEBUG ACTION_THORIUM_NODE_ADD_INITIAL_ACTORS_REPLY received from"
                        " %d, %d/%d ready\n", thorium_node_name(node),
                        source, node->ready, nodes);
#endif
        if (node->ready == nodes) {

            for (i = 0; i < nodes; i++) {
                thorium_node_send_to_node_empty(node, i, ACTION_THORIUM_NODE_START);
            }
        }

        return 1;

    } else if (tag == ACTION_THORIUM_NODE_START) {

        /* disable transport layer
         * if there is only one node
         */
        if (node->nodes == 1) {
            CORE_BITMAP_CLEAR_BIT(node->flags, FLAG_USE_TRANSPORT);
        }

#ifdef THORIUM_NODE_DEBUG_RECEIVE_SYSTEM
        printf("DEBUG node %d starts its initial actor\n",
                        thorium_node_name(node));
#endif

        /* send ACTION_START to initial actor
         * on this node
         */
        thorium_node_start_initial_actor(node);

        return 1;
    }

    return 0;
}

void thorium_node_send_to_node_empty(struct thorium_node *node, int destination, int tag)
{
    struct thorium_message message;
    thorium_message_init(&message, tag, 0, NULL);
    thorium_node_send_to_node(node, destination, &message);
}

void thorium_node_send_to_node(struct thorium_node *node, int destination,
                struct thorium_message *message)
{
    void *new_buffer;
    int count;
    size_t new_count;
    void *buffer;
    struct thorium_message new_message;
    int tag;

    tag = thorium_message_action(message);
    count = thorium_message_count(message);
    buffer = thorium_message_buffer(message);
    new_count = thorium_message_metadata_size(message) + count;

    /* the runtime system always needs at least
     * 2 int in the buffer for actor names
     * Since we are sending messages between
     * nodes, these names are faked...
     */
    new_buffer = core_memory_pool_allocate(&node->outbound_message_memory_pool, new_count);

    if (count > 0)
        core_memory_copy(new_buffer, buffer, count);

    /* the metadata size is added by the runtime
     * this is why the value is count and not new_count
     */
    thorium_message_init(&new_message, tag, count, new_buffer);
    thorium_message_set_source(&new_message, thorium_node_name(node));
    thorium_message_set_destination(&new_message, destination);
    thorium_message_write_metadata(&new_message);

#ifdef THORIUM_NODE_DEBUG
    printf("DEBUG thorium_node_send_to_node %d\n", destination);
#endif

    /*
     * Mark the message.
     */

    thorium_message_set_type(&new_message, THORIUM_MESSAGE_TYPE_NODE_OUTBOUND);

    thorium_node_send(node, &new_message); }

int thorium_node_has_actor(struct thorium_node *self, int name)
{
    int node_name;

    node_name = thorium_node_actor_node(self, name);

    if (node_name == self->name) {

            return 1;

        /* The block below is disabled.
         */
#ifdef THORIUM_NODE_CHECK_DEAD_ACTOR
        /* maybe the actor is dead already !
         */
        if (thorium_node_get_actor_from_name(self, name) != NULL) {
            return 1;
        }
#endif
    }

    return 0;
}

void thorium_node_send(struct thorium_node *node, struct thorium_message *message)
{
    int name;
    int metadata_size;
    int all;
    int count;
    int worker;
    void *buffer;
    struct thorium_worker_buffer worker_buffer;

    /*
     * Send the message through the mock transport which is
     * a transport profiler.
     */
    if (CORE_BITMAP_GET_BIT(node->flags, FLAG_PROFILE_MESSAGE_TRANSPORT)) {
        thorium_transport_profiler_send_mock(&node->transport_profiler, message);
    }

    /* Check the message to see
     * if it is a special message.
     *
     * System message have no buffer to free because they have no buffer.
     */
    if (thorium_node_send_system(node, message)) {
        return;
    }

    tracepoint(thorium_message, node_send_system, message);

    name = thorium_message_destination(message);
    thorium_node_resolve(node, message);

    /* If the actor is local, dispatch the message locally
     */
    if (thorium_node_has_actor(node, name)) {

        /* message :node_dispatch */
        tracepoint(thorium_message, node_send_dispatch, message);

        /* dispatch locally */
        thorium_node_dispatch_message(node, message);

#ifdef THORIUM_NODE_DEBUG_20140601_8
        if (thorium_message_action(message) == 1100) {
            printf("DEBUG local message 1100\n");
        }
#endif

#ifdef THORIUM_NODE_USE_COUNTERS
        core_counter_add(&node->counter, CORE_COUNTER_SENT_MESSAGES_TO_SELF, 1);
        core_counter_add(&node->counter, CORE_COUNTER_SENT_BYTES_TO_SELF,
                        thorium_message_count(message));

        core_counter_add(&node->counter, CORE_COUNTER_RECEIVED_MESSAGES_FROM_SELF, 1);
        core_counter_add(&node->counter, CORE_COUNTER_RECEIVED_BYTES_FROM_SELF,
                        thorium_message_count(message));
#endif

    /* Otherwise, the message must be sent to another BIOSAL
     * node
     */
    } else {
        /* If transport layer
         * is disable, this will never be reached anyway
         */
        /* send messages over the network */

        /*
         * Add metadata size.
         */
        count = thorium_message_count(message);
        metadata_size = thorium_message_metadata_size(message);
        all = count + metadata_size;
        thorium_message_set_count(message, all);

#ifdef TRANSPORT_DEBUG_ISSUE_594
    if (thorium_message_action(message) == 30202) {
        printf("DEBUG-594 thorium_node_send\n");
        thorium_message_print(message);
    }
#endif

        if (!thorium_message_multiplexer_multiplex(&node->multiplexer, message)) {
            thorium_node_send_with_transport(node, message);

        } else {

            /*
             * There is now a copy of the buffer.
             */
            buffer = thorium_message_buffer(message);
            worker = thorium_message_worker(message);
            thorium_worker_buffer_init(&worker_buffer, worker, buffer);
            thorium_node_inject_outbound_buffer(node, &worker_buffer);
            thorium_worker_buffer_destroy(&worker_buffer);
        }

#ifdef THORIUM_NODE_DEBUG_20140601_8
        if (thorium_message_action(message) == 1100) {
            printf("DEBUG outbound message 1100\n");

            CORE_BITMAP_SET_BIT(node->flags, FLAG_DEBUG);
        }
#endif

    }
}

struct thorium_actor *thorium_node_get_actor_from_name(struct thorium_node *node,
                int name)
{
    struct thorium_actor *actor;
    int index;

#ifdef THORIUM_NODE_DEBUG
    printf("DEBUG thorium_node_get_actor_from_name %d\n", name);
#endif

    if (name < 0) {
        return NULL;
    }

    index = thorium_node_actor_index(node, name);

    if (index < 0) {
        return NULL;
    }

    actor = (struct thorium_actor *)core_vector_at(&node->actors, index);

    return actor;
}

void thorium_node_dispatch_message(struct thorium_node *node, struct thorium_message *message)
{
    void *buffer;

    if (thorium_node_receive_system(node, message)) {

        /*
         * The buffer must be freed.
         */
        buffer = thorium_message_buffer(message);

        if (buffer != NULL) {

            /*
             * This is a Thorium core buffer since the workers are not authorized to talk to
             * the Thorium core.
             *
             * Also, this system buffer needs to be freed now anyway because there is no other
             * place where it will have the change to be freed.
             */
            core_memory_pool_free(&node->inbound_message_memory_pool, buffer);
        }

        return;
    }

    tracepoint(thorium_message, node_dispatch_message, message);

    /* otherwise, create work and dispatch work to a worker via
     * the worker pool
     */
    thorium_node_inject_message_in_worker_pool(node, message);
}

void thorium_node_inject_message_in_worker_pool(struct thorium_node *node, struct thorium_message *message)
{
    struct thorium_actor *actor;
    int name;
    int dead;

#ifdef THORIUM_NODE_DEBUG
    int tag;
    int source;
#endif

    name = thorium_message_destination(message);

#ifdef THORIUM_NODE_DEBUG
    source = thorium_message_source(message);
    tag = thorium_message_action(message);

    printf("[DEBUG %s %s %i] actor%i (node%i) : actor%i (node%i)"
                    "(tag %i) %i bytes\n",
                    __FILE__, __func__, __LINE__,
                   source, thorium_message_source_node(message),
                   name, thorium_message_destination_node(message),
                   tag, thorium_message_count(message));
#endif

    actor = thorium_node_get_actor_from_name(node, name);

    if (actor == NULL) {

#ifdef THORIUM_NODE_DEBUG_NULL_ACTOR
        printf("DEBUG node/%d: actor/%d does not exist\n", node->name,
                        name);
#endif

        thorium_node_recycle_message(node, message);
        return;
    }

    dead = thorium_actor_dead(actor);

    if (dead) {
#ifdef THORIUM_NODE_DEBUG_NULL_ACTOR
        printf("DEBUG node/%d: actor/%d is dead\n", node->name,
                        name);
#endif
        thorium_node_recycle_message(node, message);
        return;
    }

#if 0
    printf("DEBUG node enqueue message\n");
#endif
    thorium_worker_pool_enqueue_message(&node->worker_pool, message);
}

int thorium_node_actor_index(struct thorium_node *node, int name)
{
    int *bucket;
    int index;

#ifdef THORIUM_NODE_DEBUG
    printf("DEBUG calling core_map with pointer to %d, entries %d\n",
                    name, (int)core_map_table_size(&node->actor_names));
#endif

    bucket = core_map_get(&node->actor_names, &name);

#ifdef THORIUM_NODE_DEBUG
    printf("DEBUG thorium_node_actor_index %d %p\n", name, (void *)bucket);
#endif

    if (bucket == NULL) {
        return -1;
    }

    index = *bucket;
    return index;
}

int thorium_node_actor_node(struct thorium_node *node, int name)
{
    return name % node->nodes;
}

int thorium_node_name(struct thorium_node *node)
{
    return node->name;
}

int thorium_node_nodes(struct thorium_node *node)
{
    return node->nodes;
}

void thorium_node_notify_death(struct thorium_node *node, struct thorium_actor *actor)
{
    void *state;
    int name;

#ifdef THORIUM_NODE_REUSE_DEAD_INDICES
    int index;
#endif

    /* int name; */
    /*int index;*/

    /*
    node_name = node->name;
    name = thorium_actor_name(actor);
    */

    if (CORE_BITMAP_GET_BIT(node->flags, FLAG_PRINT_STRUCTURE)) {
        thorium_node_print_structure(node, actor);
    }

    name = thorium_actor_name(actor);

#ifdef THORIUM_NODE_DEBUG_SPAWN_KILL
    printf("DEBUG thorium_node_notify_death node/%d actor/%d script/%x\n",
                    thorium_node_name(node),
                    name,
                    thorium_actor_script(actor));
#endif

#ifdef THORIUM_NODE_REUSE_DEAD_INDICES
    index = thorium_node_actor_index(node, name);

#ifdef THORIUM_NODE_DEBUG_SPAWN
    printf("DEBUG node/%d THORIUM_NODE_REUSE_DEAD_INDICES push index %d\n",
                   thorium_node_name(node), index);
#endif

#endif

    state = thorium_actor_concrete_actor(actor);

#ifdef THORIUM_NODE_DEBUG_ACTOR_COUNTERS
    printf("----------------------------------------------\n");
    printf("Counters for actor/%d\n",
                    name);
    core_counter_print(thorium_actor_counter(actor));
    printf("----------------------------------------------\n");
#endif

    /*
     * Destroy the abstract actor.
     * This calls destroy on the concrete actor
     * too.
     *
     * After that, the memory can be released and
     * the actor name unregistered.
     */
    thorium_actor_destroy(actor);

    core_lock_lock(&node->spawn_and_death_lock);

    /* free the bytes of the concrete actor */
    core_memory_pool_free(&node->actor_memory_pool, state);
    state = NULL;

    /* remove the name from the registry */
    /* maybe a lock is needed for this
     * because spawn also access this attribute
     */

    core_map_delete(&node->actor_names, &name);

    /*
     * Make this change visible
     */
    core_memory_store_fence();

#ifdef THORIUM_NODE_REUSE_DEAD_INDICES
    core_queue_enqueue(&node->dead_indices, &index);
#endif

#ifdef THORIUM_NODE_DEBUG_20140601_8
    printf("DEBUG thorium_node_notify_death\n");
#endif

    /* Remove the actor from the list of auto-scaling
     * actors
     */

    core_lock_lock(&node->auto_scaling_lock);

    if (core_set_find(&node->auto_scaling_actors, &name)) {
        core_set_delete(&node->auto_scaling_actors, &name);

        /* This fence is not required.
         */
        /*
        core_memory_store_fence();
        */
    }

    core_lock_unlock(&node->auto_scaling_lock);

    node->alive_actors--;
    node->dead_actors++;

    if (node->alive_actors == 0
                    && CORE_BITMAP_GET_BIT(node->flags, FLAG_PRINT_LOAD)) {

        printf("thorium_node: all local actors are dead now, %d alive actors, %d dead actors\n",
                        node->alive_actors, node->dead_actors);
    }

#ifdef THORIUM_NODE_USE_COUNTERS
    core_counter_add(&node->counter, CORE_COUNTER_KILLED_ACTORS, 1);
#endif

    core_lock_unlock(&node->spawn_and_death_lock);

#ifdef THORIUM_NODE_DEBUG_20140601_8
    printf("DEBUG exiting thorium_node_notify_death\n");
#endif
}

int thorium_node_worker_count(struct thorium_node *node)
{
    return thorium_worker_pool_worker_count(&node->worker_pool);
}

int thorium_node_argc(struct thorium_node *node)
{
    return node->argc;
}

char **thorium_node_argv(struct thorium_node *node)
{
    return node->argv;
}

int thorium_node_thread_count(struct thorium_node *node)
{
    return node->threads;
}

void thorium_node_add_script(struct thorium_node *node, int name,
                struct thorium_script *script)
{
    int can_add;

    core_lock_lock(&node->script_lock);

    can_add = 1;
    if (thorium_node_has_script(node, script)) {
        can_add = 0;
    }

    if (can_add) {
        core_map_add_value(&node->scripts, &name, &script);
    }

#ifdef THORIUM_NODE_DEBUG_SCRIPT_SYSTEM
    printf("DEBUG added script %x %p\n", name, (void *)script);
#endif

    core_lock_unlock(&node->script_lock);
}

int thorium_node_has_script(struct thorium_node *node, struct thorium_script *script)
{
    if (thorium_node_find_script(node, thorium_script_identifier(script)) != NULL) {
        return 1;
    }
    return 0;
}

struct thorium_script *thorium_node_find_script(struct thorium_node *node, int identifier)
{
    struct thorium_script **script;

    script = core_map_get(&node->scripts, &identifier);

    if (script == NULL) {
        return NULL;
    }

    return *script;
}

#ifdef THORIUM_NODE_USE_COUNTERS
void thorium_node_print_counters(struct thorium_node *node)
{
    printf("----------------------------------------------\n");
    printf("thorium_node: counters for node/%d\n",
                    thorium_node_name(node));
    core_counter_print(&node->counter, thorium_node_name(node));
}
#endif

/*
 * That would be cool to be able to survive a segmentation fault.
 * This could be done by killing the actor and continue
 * computation.
 */
void thorium_node_handle_signal(int signal)
{
    int node;
    struct thorium_node *self;

    self = thorium_node_global_self;

    node = thorium_node_name(self);

    if (signal == SIGSEGV) {
        printf("Error, node/%d received signal SIGSEGV\n", node);

    } else if (signal == SIGUSR1) {
        thorium_node_toggle_debug_mode(thorium_node_global_self);
        return;
    } else {
        printf("Error, node/%d received signal %d\n", node, signal);
    }

    /*
     * Examine everything.
     */
    thorium_node_examine(thorium_node_global_self);

    /*
     * Print call stack.
     */
    core_tracer_print_stack_backtrace();

    fflush(stdout);

    /* remove handler
     * \see http://stackoverflow.com/questions/9302464/how-do-i-remove-a-signal-handler
     */

    self->action.sa_handler = SIG_DFL;
    sigaction(signal, &self->action, NULL);
}

void thorium_node_register_signal_handlers(struct thorium_node *self)
{
    struct core_vector signals;
    struct core_vector_iterator iterator;
    int *signal;

    core_vector_init(&signals, sizeof(int));
    /*
     * \see http://unixhelp.ed.ac.uk/CGI/man-cgi?signal+7
     */
    /* segmentation fault */
    core_vector_push_back_int(&signals, SIGSEGV);
    /* division by 0 */
    core_vector_push_back_int(&signals, SIGFPE);
    /* bus error (alignment issue */
    core_vector_push_back_int(&signals, SIGBUS);
    /* abort */
    core_vector_push_back_int(&signals, SIGABRT);

    core_vector_push_back_int(&signals, SIGUSR1);

#if 0
    /* kill signal */
    core_vector_push_back_int(&signals, SIGKILL);
    /* termination*/
    core_vector_push_back_int(&signals, SIGTERM);

    /* interruption */
    core_vector_push_back_int(&signals, SIGINT);
    /* kill */
    core_vector_push_back_int(&signals, SIGHUP);
    /* illegal instruction */
    core_vector_push_back_int(&signals, SIGILL);
#endif

    /*
     * \see http://pubs.opengroup.org/onlinepubs/7908799/xsh/sigaction.html
     * \see http://stackoverflow.com/questions/10202941/segmentation-fault-handling
     */
    self->action.sa_handler = thorium_node_handle_signal;
    sigemptyset(&self->action.sa_mask);
    self->action.sa_flags = 0;

    core_vector_iterator_init(&iterator, &signals);

    while (core_vector_iterator_has_next(&iterator)) {

        core_vector_iterator_next(&iterator, (void **)&signal);
        sigaction(*signal, &self->action, NULL);
    }

    core_vector_iterator_destroy(&iterator);
    core_vector_destroy(&signals);

        /* generate SIGSEGV
    *((int *)NULL) = 0;
     */
}

void thorium_node_print_structure(struct thorium_node *node, struct thorium_actor *actor)
{
    struct core_map *structure;
    struct core_map_iterator iterator;
    int *source;
    int *count;
    int name;
    int script;
    struct thorium_script *actual_script;
    char color[32];
    int node_name;

    node_name = thorium_node_name(node);
    name = thorium_actor_name(actor);
    script = thorium_actor_script(actor);
    actual_script = thorium_node_find_script(node, script);

    if (node_name == 0) {
        strcpy(color, "red");
    } else if (node_name == 1) {
        strcpy(color, "green");
    } else if (node_name == 2) {
        strcpy(color, "blue");
    } else if (node_name == 3) {
        strcpy(color, "pink");
    }

    structure = thorium_actor_get_received_messages(actor);

    printf("    a%d [label=\"%s/%d\", color=\"%s\"]; /* STRUCTURE vertex */\n", name,
                    thorium_script_description(actual_script), name, color);

    core_map_iterator_init(&iterator, structure);

    while (core_map_iterator_has_next(&iterator)) {
        core_map_iterator_next(&iterator, (void **)&source, (void **)&count);

        printf("    a%d -> a%d [label=\"%d\"]; /* STRUCTURE edge */\n", *source, name, *count);
    }

    printf(" /* STRUCTURE */\n");

    core_map_iterator_destroy(&iterator);
}

struct thorium_worker_pool *thorium_node_get_worker_pool(struct thorium_node *self)
{
    return &self->worker_pool;
}

void thorium_node_toggle_debug_mode(struct thorium_node *self)
{
    if (CORE_BITMAP_GET_BIT(self->flags, FLAG_DEBUG)) {
        CORE_BITMAP_CLEAR_BIT(self->flags, FLAG_DEBUG);
    } else {
        CORE_BITMAP_SET_BIT(self->flags, FLAG_DEBUG);
    }
    thorium_worker_pool_toggle_debug_mode(&self->worker_pool);
}

void thorium_node_reset_actor_counters(struct thorium_node *node)
{
    struct core_map_iterator iterator;
    int *name;
    struct thorium_actor *actor;

    core_map_iterator_init(&iterator, &node->actor_names);

    while (core_map_iterator_next(&iterator, (void **)&name, NULL)) {

        actor = thorium_node_get_actor_from_name(node, *name);

        thorium_actor_reset_counters(actor);
    }
    core_map_iterator_destroy(&iterator);
}

int64_t thorium_node_get_counter(struct thorium_node *node, int counter)
{
#ifdef THORIUM_NODE_USE_COUNTERS
    return core_counter_get(&node->counter, counter);
#else
    return 0;
#endif
}

int thorium_node_send_system(struct thorium_node *node, struct thorium_message *message)
{
    int destination;
    int tag;
    int source;

    tag = thorium_message_action(message);
    destination = thorium_message_destination(message);
    source = thorium_message_source(message);

    if (source == destination
            && tag == ACTION_ENABLE_AUTO_SCALING) {

        printf("AUTO-SCALING node/%d enables auto-scaling for actor %d (ACTION_ENABLE_AUTO_SCALING)\n",
                       thorium_node_name(node),
                       source);

        core_lock_lock(&node->auto_scaling_lock);

        core_set_add(&node->auto_scaling_actors, &source);

        core_lock_unlock(&node->auto_scaling_lock);

        return 1;

    } else if (source == destination
           && tag == ACTION_DISABLE_AUTO_SCALING) {

        core_lock_lock(&node->auto_scaling_lock);

        core_set_delete(&node->auto_scaling_actors, &source);

        core_lock_unlock(&node->auto_scaling_lock);

        return 1;
    }

    return 0;
}


void thorium_node_send_to_actor(struct thorium_node *node, int name, struct thorium_message *message)
{
    thorium_message_set_source(message, name);
    thorium_message_set_destination(message, name);

    thorium_node_send(node, message);
}

void thorium_node_check_load(struct thorium_node *node)
{
    const float load_threshold = 0.90;
    struct core_set_iterator iterator;
    struct thorium_message message;
    int name;
    time_t current_time;

    current_time = time(NULL);

    /* Do the auto-scaling thing one time m aximum
     * for each second
     */
    if (current_time == node->last_auto_scaling) {
        return;
    }

    node->last_auto_scaling = current_time;

    /* Check load to see if auto-scaling is needed
     */

    core_lock_lock(&node->auto_scaling_lock);

    if (thorium_worker_pool_get_current_load(&node->worker_pool)
                    <= load_threshold) {


        core_set_iterator_init(&iterator, &node->auto_scaling_actors);

        while (core_set_iterator_get_next_value(&iterator, &name)) {

            thorium_message_init(&message, ACTION_DO_AUTO_SCALING,
                            0, NULL);

            thorium_node_send_to_actor(node, name, &message);

            thorium_message_destroy(&message);
        }

        core_set_iterator_destroy(&iterator);
    }

    core_lock_unlock(&node->auto_scaling_lock);
}

void thorium_node_run_loop(struct thorium_node *node)
{
    struct thorium_message message;
    int credits;
    int starting_credits;
    void *buffer;
    char send_in_thread;
    char use_transport;
    char run_in_main_thread;

#ifdef THORIUM_NODE_ENABLE_INSTRUMENTATION
    int period;
    time_t current_time;
    char print_information = 0;

    if (CORE_BITMAP_GET_BIT(node->flags, FLAG_PRINT_LOAD)
            || CORE_BITMAP_GET_BIT(node->flags, FLAG_PRINT_COUNTERS)) {

        print_information = 1;
    }

    period = THORIUM_NODE_LOAD_PERIOD;
#endif

    starting_credits = 1024;
    credits = starting_credits;

    send_in_thread = CORE_BITMAP_GET_BIT(node->flags,
                                FLAG_SEND_IN_THREAD);
    use_transport = CORE_BITMAP_GET_BIT(node->flags, FLAG_USE_TRANSPORT);
    run_in_main_thread = CORE_BITMAP_GET_BIT(node->flags, FLAG_WORKER_IN_MAIN_THREAD);

    while (credits > 0) {

        tracepoint(thorium_node, run_loop_print, node->name, node->tick);

        CORE_DEBUGGER_JITTER_DETECTION_START(node_main_loop);

        CORE_DEBUGGER_JITTER_DETECTION_START(node_print);

#ifdef THORIUM_NODE_ENABLE_INSTRUMENTATION
        if (print_information) {
            current_time = time(NULL);

            if (current_time - node->last_report_time >= period) {
                if (CORE_BITMAP_GET_BIT(node->flags, FLAG_PRINT_LOAD)) {

                    thorium_worker_pool_print_load(&node->worker_pool, THORIUM_WORKER_POOL_LOAD_EPOCH);

                    /* Display the number of actors,
                     * the number of active buffers/requests/messages,
                     * and
                     * the heap size.
                     */
                    printf("thorium_node: node/%d METRICS AliveActorCount: %d ByteCount: %" PRIu64 " / %" PRIu64 "\n",
                                    node->name,
                                    node->alive_actors,
                                    core_memory_get_utilized_byte_count(),
                                    core_memory_get_total_byte_count());

                    printf("thorium_node: node/%d MESSAGES"
                                    " ReceivedMessageCount: %" PRIu64 ""
                                    " SentMessageCount: %" PRIu64 ""
                                    " BufferedInboundMessageCount: %d"
                                    " ActiveRequestCount: %d"
                                    "\n",
                                    node->name,
                                    core_counter_get(&node->counter, CORE_COUNTER_RECEIVED_MESSAGES),
                                    core_counter_get(&node->counter, CORE_COUNTER_SENT_MESSAGES),
                                    thorium_worker_pool_buffered_message_count(&node->worker_pool),
                                    thorium_transport_get_active_request_count(&node->transport)
                                    );
                }

#ifdef THORIUM_NODE_USE_COUNTERS
                if (CORE_BITMAP_GET_BIT(node->flags, FLAG_PRINT_COUNTERS)) {
                    thorium_node_print_counters(node);
                }
#endif

                node->last_report_time = current_time;
            }
        }
#endif
        CORE_DEBUGGER_JITTER_DETECTION_END(node_print, 0);

#ifdef THORIUM_NODE_USE_TICKS
        if (worker->tick_count % 1000000 == 0) {
            thorium_node_print_counters(node);
        }
#endif

#ifdef THORIUM_NODE_DEBUG_LOOP1
        if (CORE_BITMAP_GET_BIT(node->flags, FLAG_DEBUG)) {
            printf("DEBUG node/%d is running\n", thorium_node_name(node));
        }
#endif

        CORE_DEBUGGER_JITTER_DETECTION_START(node_receive);

        /* pull message from network and assign the message to a thread.
         * this code path will call lock if
         * there is a message received.
         */
        if (use_transport
                        && !thorium_node_regulator_must_wait(node)
                        && thorium_transport_receive(&node->transport, &message)) {

#ifdef THORIUM_NODE_DEBUG_INJECTION
            /*
             * This gives a list of inbound buffers allocated by the node.
             */
            ++node->counter_allocated_node_inbound_buffers;
#endif
            tracepoint(thorium_message, node_receive, &message);

#ifdef THORIUM_NODE_USE_COUNTERS
            core_counter_add(&node->counter, CORE_COUNTER_RECEIVED_MESSAGES_NOT_FROM_SELF, 1);
            core_counter_add(&node->counter, CORE_COUNTER_RECEIVED_BYTES_NOT_FROM_SELF,
                    thorium_message_count(&message));
#endif

            if (!thorium_message_multiplexer_demultiplex(&node->multiplexer, &message)) {
                thorium_node_dispatch_message(node, &message);
            } else {
                /*
                 * Don't leak memory
                 */
#ifdef THORIUM_NODE_DEBUG_INJECTION
                ++node->counter_freed_multiplexed_inbound_buffers;
#endif

                buffer = thorium_message_buffer(&message);
                core_memory_pool_free(&node->inbound_message_memory_pool, buffer);
            }
        }

        tracepoint(thorium_node, run_loop_receive, node->name, node->tick);

        CORE_DEBUGGER_JITTER_DETECTION_END(node_receive, 0);

        /* the one worker works here if there is only
         * one thread
         */
        if (run_in_main_thread) {
            thorium_worker_pool_run(&node->worker_pool);
        }

        tracepoint(thorium_node, run_loop_run, node->name, node->tick);

        CORE_DEBUGGER_JITTER_DETECTION_START(node_send);

        /* with 3 or more threads, the sending operations are
         * in another thread */
        if (!send_in_thread) {

#ifdef THORIUM_NODE_DEBUG_RUN
            if (node->alive_actors == 0) {
                printf("THORIUM_NODE_DEBUG_RUN sending messages, no local actors\n");
            }
#endif
            thorium_node_send_messages(node);
        }

        tracepoint(thorium_node, run_loop_send_after, node->name, node->tick);

        CORE_DEBUGGER_JITTER_DETECTION_END(node_send, 0);

        CORE_DEBUGGER_JITTER_DETECTION_START(node_pool_work);

        /* Flush queue buffers in the worker pool
         */

        thorium_worker_pool_work(&node->worker_pool);

        tracepoint(thorium_node, run_loop_pool_work, node->name, node->tick);

        --credits;

        /* if the node is still running, allocate new credits
         * to the engine loop
         */
        if (credits == 0) {
            if (thorium_node_running(node)) {
                credits = starting_credits;
#ifdef THORIUM_NODE_DEBUG_RUN
                printf("THORIUM_NODE_DEBUG_RUN here are some credits\n");
#endif
            }

            thorium_node_check_load(node);
        }

        CORE_DEBUGGER_JITTER_DETECTION_END(node_pool_work, 0);

        CORE_DEBUGGER_JITTER_DETECTION_START(node_test);

        /* Free buffers of active requests
         */
        if (use_transport) {

            thorium_node_test_requests(node);

            thorium_message_multiplexer_test(&node->multiplexer);
        }

        tracepoint(thorium_node, run_loop_test_requests, node->name, node->tick);

        thorium_node_do_message_triage(node);

#ifdef THORIUM_NODE_DEBUG_RUN
        if (node->alive_actors == 0) {
            printf("THORIUM_NODE_DEBUG_RUN credits: %d\n", credits);
        }
#endif

        tracepoint(thorium_node, run_loop_do_triage, node->name, node->tick);

        CORE_DEBUGGER_JITTER_DETECTION_END(node_test, 0);

        CORE_DEBUGGER_JITTER_DETECTION_END(node_main_loop, 0);

        thorium_node_regulator_run(node);

#ifdef THORIUM_NODE_USE_TICKS
        ++node->tick_count;
#endif
        ++node->tick;
    }

#ifdef THORIUM_NODE_DEBUG_20140601_8
    printf("DEBUG node/%d exited loop\n",
                    thorium_node_name(node));
#endif
}

void thorium_node_send_messages(struct thorium_node *node)
{
    struct thorium_message message;
    int i;
    int count;

    i = 0;
    /*
    count = node->worker_count;
    */

    /*
     * This was tested with 1 node with 28 worker threads.
     *
     * When running in distributed setup, the ring won't have
     * 256 elements most of the time since there are data
     * dependencies with the outside world.
     */
    count = 128;

    /*
    if (count == 0)
        count = 1;
        */

    tracepoint(thorium_node, send_messages_enter, node->name, node->tick);

    /*
     * Check for messages to send from from worker threads.
     * The loop pulls at most <count> messages.
     *
     * This loop is lockless.
     */
    while (i < count && thorium_worker_pool_dequeue_message(&node->worker_pool, &message)) {

        ++i;

        tracepoint(thorium_message, node_send, &message);

        /*
        thorium_message_set_count(message,
                    thorium_message_count(message) - THORIUM_MESSAGE_METADATA_SIZE);
                    */

        /*
        thorium_message_set_count(message,
                    thorium_message_count(message) + THORIUM_MESSAGE_METADATA_SIZE);
        */

#ifdef THORIUM_NODE_DEBUG
        printf("thorium_node_run pulled tag %i buffer %p\n",
                        thorium_message_action(&message),
                        thorium_message_buffer(&message));
#endif

#ifdef THORIUM_NODE_DEBUG_RUN
        if (node->alive_actors == 0) {
            printf("THORIUM_NODE_DEBUG_RUN thorium_node_send_messages pulled a message, tag %d\n",
                            thorium_message_action(&message));
        }
#endif

        /*
         * Send it locally or over the network
         */
        thorium_node_send(node, &message);

#if 0
    } else {
#ifdef THORIUM_NODE_DEBUG_RUN
        if (node->alive_actors == 0) {
            printf("THORIUM_NODE_DEBUG_RUN thorium_node_send_messages no message\n");
        }
#endif
#endif
    }

    if (i)
        node->last_transport_event_time = time(NULL);

    tracepoint(thorium_node, send_messages_exit, node->name, node->tick);
}

void thorium_node_test_requests(struct thorium_node *node)
{
    struct thorium_worker_buffer worker_buffer;
    int requests;
    int requests_to_test;
    int i;
    int maximum;
    /*
    int worker;
    */

    /*
     * Use a half-life approach
     */
    requests = thorium_transport_get_active_request_count(&node->transport);

    /*
     * Nothing to do.
     */
    if (requests == 0)
        return;

    requests_to_test = requests;

    /*
     * Make sure the amount is within the bounds.
     */
    maximum = 64;
    if (requests_to_test > maximum)
        requests_to_test = maximum;

    /* Test active buffer requests
     */
    i = 0;
    while (i < requests_to_test) {
        if (thorium_transport_test(&node->transport, &worker_buffer)) {

#if 0
            worker = thorium_worker_buffer_get_worker(&worker_buffer);
#endif

            thorium_node_inject_outbound_buffer(node, &worker_buffer);
        }

        ++i;
    }

#ifdef THORIUM_NODE_INJECT_CLEAN_WORKER_BUFFERS
    /* Check if there are queued buffers to give to workers
     */
    if (core_fast_queue_dequeue(&node->output_clean_outbound_buffer_queue, &worker_buffer)) {

            /*
        printf("INJECT Dequeued buffer to inject\n");
        */
        thorium_node_free_worker_buffer(node, &worker_buffer);
    }
#endif
}

void thorium_node_free_worker_buffer(struct thorium_node *node,
                struct thorium_worker_buffer *worker_buffer)
{
    void *buffer;

#ifdef THORIUM_NODE_INJECT_CLEAN_WORKER_BUFFERS
    int worker_name;
    struct thorium_worker *worker;
#endif

    buffer = thorium_worker_buffer_get_buffer(worker_buffer);

#ifdef THORIUM_NODE_INJECT_CLEAN_WORKER_BUFFERS
    worker_name = thorium_worker_buffer_get_worker(worker_buffer);

    CORE_DEBUGGER_ASSERT(worker_name >= 0);

    /* This a worker buffer.
     * Otherwise, this is a Thorium node for startup.
     */
    worker = thorium_worker_pool_get_worker(&node->worker_pool, worker_name);

    CORE_DEBUGGER_ASSERT(worker != NULL);

    /*
    printf("INJECT Injecting buffer into worker\n");
    */

    /* Push the buffer in the ring of the worker
     */
    if (!thorium_worker_inject_clean_outbound_buffer(worker, buffer)) {

        /* If the ring is full, queue it locally
         * and try again later
         */
        core_fast_queue_enqueue(&node->output_clean_outbound_buffer_queue, worker_buffer);
    }

/* This is a node buffer
 * (for startup)
 */
#endif
}

void thorium_node_do_message_triage(struct thorium_node *self)
{
    struct thorium_worker *worker;
    struct thorium_message message;

    /*
     * First, verify if any message needs to be processed from
     * the worker pool.
     *
     * Such messages are for example messages that could not be sent to a
     * given already-dead actor.
     */
    if (thorium_worker_pool_dequeue_message_for_triage(&self->worker_pool, &message)) {

        thorium_node_recycle_message(self, &message);
    }

    worker = thorium_worker_pool_get_worker(&self->worker_pool, self->worker_for_triage);

    /*
     * Position the index on the next worker to process.
     */
    ++self->worker_for_triage;
    if (self->worker_for_triage == self->worker_count) {
        self->worker_for_triage = 0;
    }

    if (thorium_worker_dequeue_message_for_triage(worker, &message)) {

        thorium_node_recycle_message(self, &message);
    }
}

void thorium_node_recycle_message(struct thorium_node *self, struct thorium_message *message)
{
    int worker_name;
    void *buffer;
    struct thorium_worker_buffer worker_buffer;

    /*
     * This function recycle a message.
     * The type can not be THORIUM_MESSAGE_TYPE_NODE_OUTBOUND.
     */

#ifdef CORE_DEBUGGER_ENABLE_ASSERT
    if (!(thorium_message_type(message) == THORIUM_MESSAGE_TYPE_NODE_INBOUND
                    || thorium_message_type(message) == THORIUM_MESSAGE_TYPE_WORKER_OUTBOUND)) {
        printf("Error: invalid type in thorium_node_recycle_message, actual: %d\n",
                        thorium_message_type(message));
        thorium_message_print(message);
    }
#endif

    /*
     * It is not clear when a message can be passed with the type
     * THORIUM_MESSAGE_TYPE_NONE.
     */
    CORE_DEBUGGER_ASSERT(thorium_message_type(message) == THORIUM_MESSAGE_TYPE_NODE_INBOUND
                    || thorium_message_type(message) == THORIUM_MESSAGE_TYPE_WORKER_OUTBOUND);

    worker_name = thorium_message_worker(message);
    buffer = thorium_message_buffer(message);

    CORE_DEBUGGER_ASSERT(buffer != NULL);

    /*
     * Otherwise, free the buffer here directly since this is a Thorium core
     * buffer for startup.
     */
    if (worker_name < 0) {

        core_memory_pool_free(&self->inbound_message_memory_pool, buffer);
#ifdef THORIUM_NODE_DEBUG_INJECTION
        ++self->counter_freed_injected_node_inbound_buffers;
#endif

    } else {

        /*
         * Otherwise, this is a worker buffer.
         */
        thorium_worker_buffer_init(&worker_buffer, worker_name, buffer);
        thorium_node_free_worker_buffer(self, &worker_buffer);

#ifdef THORIUM_NODE_DEBUG_INJECTION
        ++self->counter_injected_buffers_for_local_workers;
#endif

        thorium_worker_buffer_destroy(&worker_buffer);
    }
}

void thorium_node_prepare_received_message(struct thorium_node *self, struct thorium_message *message)
{
    int metadata_size;
    int count;

    /*
     * Remove the metadata from the count because
     * actors don't need that.
     */
    count = thorium_message_count(message);
    metadata_size = thorium_message_metadata_size(message);
    count -= metadata_size;
    thorium_message_set_count(message, count);
    thorium_message_read_metadata(message);
    thorium_node_resolve(self, message);
}

void thorium_node_resolve(struct thorium_node *self, struct thorium_message *message)
{
    int actor;
    int node_name;
    struct thorium_node *node;

    node = self;

    actor = thorium_message_source(message);
    node_name = thorium_node_actor_node(node, actor);
    thorium_message_set_source_node(message, node_name);

    actor = thorium_message_destination(message);
    node_name = thorium_node_actor_node(node, actor);
    thorium_message_set_destination_node(message, node_name);
}

int thorium_node_generate_random_name(struct thorium_node *self,
                int minimal_value, int maximum_value)
{
#if defined(THORIUM_NODE_USE_DETERMINISTIC_ACTOR_NAMES)

    int name;
    int stride;

    if (self->current_actor_name < minimal_value
                    || self->current_actor_name > maximum_value
                    || self->current_actor_name == THORIUM_ACTOR_NOBODY) {

        self->current_actor_name = minimal_value;
    }

    stride = thorium_node_nodes(self);

    name = self->current_actor_name;
    self->current_actor_name += stride;

    return name;

#elif defined(USE_RANDOM_ACTOR_NAMES)
    /*
     * On BGQ, RAND_MAX is:
     *
     * > [harms@vestalac1 ~]$ echo -e "#include <stdlib.h>\nunsigned long long
     * > rand_max = RAND_MAX;" | powerpc64-bgq-linux-gcc -E - | tail -1
     * > unsigned long long rand_max = 2147483647;Q
     *
     * (thanks to Harms, Kevin N. at ALCF)
     */
    int name;
    int range;

    range = maximum_value - minimal_value;
    name = rand() % range + minimal_value;
    return name;


#endif
}

void thorium_node_send_with_transport(struct thorium_node *self, struct thorium_message *message)
{
    thorium_transport_send(&self->transport, message);

#ifdef THORIUM_NODE_USE_COUNTERS
    core_counter_add(&self->counter, CORE_COUNTER_SENT_MESSAGES_NOT_TO_SELF, 1);
    core_counter_add(&self->counter, CORE_COUNTER_SENT_BYTES_NOT_TO_SELF,
                        thorium_message_count(message));
#endif
}

struct core_memory_pool *thorium_node_inbound_memory_pool(struct thorium_node *self)
{
    return &self->inbound_message_memory_pool;
}

void thorium_node_examine(struct thorium_node *self)
{
    printf("DEBUG_NODE Name= %d WorkerCount= %d\n",
                    self->name,
            thorium_worker_pool_worker_count(&self->worker_pool));

    printf("MEMORY used / total -> %" PRIu64 " / %" PRIu64  "\n",
                        core_memory_get_utilized_byte_count(),
                        core_memory_get_total_byte_count());

    core_memory_pool_examine(&self->actor_memory_pool);
    core_memory_pool_examine(&self->inbound_message_memory_pool);
    core_memory_pool_examine(&self->outbound_message_memory_pool);

    thorium_worker_pool_examine(&self->worker_pool);
}

void thorium_node_inject_outbound_buffer(struct thorium_node *self, struct thorium_worker_buffer *worker_buffer)
{
    int worker;
    void *buffer;

    worker = thorium_worker_buffer_get_worker(worker_buffer);

    /*
     * This buffer was allocated by the Thorium core and not by
     * a worker.
     */
    if (worker < 0) {

        buffer = thorium_worker_buffer_get_buffer(worker_buffer);
        core_memory_pool_free(&self->outbound_message_memory_pool,
                                buffer);
#ifdef THORIUM_NODE_DEBUG_INJECTION
        ++self->counter_freed_thorium_outbound_buffers;
#endif

#ifdef THORIUM_NODE_DEBUG_INJECTION_DETAILS
        printf("Worker= %d\n", worker);
#endif

    } else {

        /*
         * The buffer was allocated by a worker.
         */

        thorium_node_free_worker_buffer(self, worker_buffer);

#ifdef THORIUM_NODE_DEBUG_INJECTION
        ++self->counter_injected_transport_outbound_buffer_for_workers;
#endif
    }
}

void thorium_node_regulator_configure(struct thorium_node *self)
{
#ifdef THORIUM_NODE_CONFIG_USE_REGULATOR
    CORE_BITMAP_CLEAR_BIT(self->flags, FLAG_REGULATOR_IS_ENABLED);
#endif
}

void thorium_node_regulator_run(struct thorium_node *self)
{
#ifdef THORIUM_NODE_CONFIG_USE_REGULATOR
    int count;
    int minimum;

    minimum = 512;

    count = thorium_worker_pool_buffered_message_count(&self->worker_pool);

    /*
     * When the minimum is reached, activate the regulator.
     */
    if (count >= minimum) {

        /*
         * First, check if the bit is not already set.
         */
        if (!CORE_BITMAP_GET_BIT(self->flags, FLAG_REGULATOR_IS_ENABLED)) {

            CORE_BITMAP_SET_BIT(self->flags, FLAG_REGULATOR_IS_ENABLED);

#ifdef DEBUG_NODE_REGULATOR
            printf("thorium_node: Warning buffered_message_count= %d FLAG_REGULATOR_IS_ENABLED = 1\n",
                            count);
#endif
        }

    /*
     * Otherwise, the queue size is below the minimum required to
     * activate the regulator.
     *
     * In that case, the regulator must be disabled.
     */
    } else {

        /*
         * Clear the big FLAG_REGULATOR_IS_ENABLED if it is set.
         */
        if (CORE_BITMAP_GET_BIT(self->flags, FLAG_REGULATOR_IS_ENABLED)) {
            CORE_BITMAP_CLEAR_BIT(self->flags, FLAG_REGULATOR_IS_ENABLED);

#ifdef DEBUG_NODE_REGULATOR
            printf("thorium_node: Warning buffered_message_count= %d FLAG_REGULATOR_IS_ENABLED = 0\n",
                            count);
#endif
        }
    }
#endif
}

int thorium_node_regulator_must_wait(struct thorium_node *self)
{
#ifdef THORIUM_NODE_CONFIG_USE_REGULATOR
    return CORE_BITMAP_GET_BIT(self->flags, FLAG_REGULATOR_IS_ENABLED);
#else
    return 0;
#endif
}
