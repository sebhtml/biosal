
#ifndef THORIUM_NODE_H
#define THORIUM_NODE_H

#include "actor.h"
#include "worker_pool.h"

#include "transport/transport.h"
#include "transport/message_multiplexer.h"
#include "transport/multiplexer_policy.h"

#include <core/structures/vector.h>
#include <core/structures/queue.h>
#include <core/structures/map.h>

#include <core/system/lock.h>
#include <core/system/counter.h>
#include <core/system/memory_pool.h>

/*
#define THORIUM_NODE_DEBUG_INJECTION
*/

/*
 * \see http://pubs.opengroup.org/onlinepubs/009696699/basedefs/signal.h.html
 */
#include <signal.h>

/* Some message tags at the node level instead of the actor level
 */

#define ACTION_THORIUM_NODE_ADD_INITIAL_ACTOR 0x00002438
#define ACTION_THORIUM_NODE_ADD_INITIAL_ACTORS 0x00004c19
#define ACTION_THORIUM_NODE_ADD_INITIAL_ACTORS_REPLY 0x00003ad3
#define ACTION_THORIUM_NODE_START 0x0000082c

/*
 * Use deterministic actor names.
 */
#define THORIUM_NODE_USE_DETERMINISTIC_ACTOR_NAMES

/*
 * Thorium product branding.
 */
#define THORIUM_NODE_THORIUM_PREFIX "[thorium]"

/*
 * Compilation options:
 */

/*
 * Enable load reporting with -print-load and memory reporting with
 * -print-memory-usage
 */
/*
 */
#define THORIUM_NODE_ENABLE_INSTRUMENTATION
#define THORIUM_NODE_LOAD_PERIOD 10

/*
*/

struct thorium_script;
struct thorium_worker_buffer;

/*
 * This is the Thorium distributed actor engine developed
 * at Argonne National Laboratory.
 *
 * BIOSAL applications are developed by creating actors
 * and by running them in Thorium.
 *
 * Thorium has these components:
 *
 * - Runtime node (struct thorium_node)
 * - Actor scheduler (struct thorium_scheduler)
 * - Actor (struct thorium_actor)
 * - Message (struct thorium_message)
 * - Worker pool (struct thorium_worker_pool)
 * - Worker (struct thorium_worker)
 * - Actor script (struct thorium_script)
 *
 * - message reception: one queue per thread
 * - the queue has variables for its heads
 * - queue is implemented as linked list of arrays
 *
 * - message send process: one queue per thread
 * - for states,  use OR (|)
 * - use actor affinity in implementation
 *
 */
struct thorium_node {
    struct bsal_vector actors;
    struct bsal_set auto_scaling_actors;
    struct thorium_worker_pool worker_pool;
    struct bsal_map actor_names;
    struct bsal_vector initial_actors;
    int received_initial_actors;
    int ready;

    uint32_t flags;

    /*
     * Last number of active requests.
     */
    int last_active_request_count;

#ifdef THORIUM_NODE_INJECT_CLEAN_WORKER_BUFFERS
    struct bsal_fast_queue clean_outbound_buffers_to_inject;

    int worker_for_triage;
#endif

    struct bsal_map scripts;
    int available_scripts;
    int maximum_scripts;

/*
 * Enable a check to avoid calling transport probing function
 * when running on one single node.
 */
    struct bsal_thread thread;
    struct thorium_transport transport;
    struct thorium_message_multiplexer multiplexer;
    struct thorium_multiplexer_policy multiplexer_policy;

    /*
     * This lock can not be removed because
     * of the function thorium_actor_spawn.
     * The message ACTION_SPAWN however does not
     * requires locking.
     *
     * If thorium_actor_spawn is removed from the API, then
     * this lock might be removed.
     */
    struct bsal_lock spawn_and_death_lock;

    /*
     * This lock is required because of the
     * function thorium_actor_add_script.
     *
     * A message tag THORIUM_ACTOR_ADD_SCRIPT could be added
     * in order to remove this lock.
     */
    struct bsal_lock script_lock;

    /*
     * This lock is required because it is accessed when
     * thorium_node_notify_death is called from thorium_actor_die.
     *
     * This could be fixed by changing the semantics of ACTION_STOP.
     * Instead of catching it inside an actor, the actor could just send it to itself
     * and the Thorium engine could catch it and kill the actor
     * (the Thorium pacing thread would call thorium_node_notify_death
     * instead of the worker thread).
     */
    struct bsal_lock auto_scaling_lock;

    /*
     * Memory pool for concrete actors.
     */
    struct bsal_memory_pool actor_memory_pool;

    /*
     * Memory pool for inbound messages using the transport
     * system.
     */
    struct bsal_memory_pool inbound_message_memory_pool;

    /*
     * Memory pool for outboud messages that are not allocated by
     * workers.
     */
    struct bsal_memory_pool outbound_message_memory_pool;

    struct bsal_queue dead_indices;

    int provided;

    int name;
    int nodes;
    int threads;

    int dead_actors;
    int alive_actors;

    int argc;
    char **argv;

    struct bsal_counter counter;

    /*
     * Requires -D_POSIX_C_SOURCE=200112L
     */
    struct sigaction action;

    /*
     * Some time variables
     */
    time_t start_time;
    time_t last_report_time;
    time_t last_auto_scaling;
    time_t last_transport_event_time;

#ifdef THORIUM_NODE_USE_DETERMINISTIC_ACTOR_NAMES
    int current_actor_name;
#endif

#ifdef THORIUM_NODE_DEBUG_INJECTION
    int counter_freed_thorium_outbound_buffers;
    int counter_freed_injected_inbound_core_buffers;
    int counter_injected_buffers_for_local_workers;
    int counter_injected_transport_outbound_buffer_for_workers;
#endif
};

extern struct thorium_node *thorium_node_global_self;

void thorium_node_init(struct thorium_node *self, int *argc, char ***argv);
void thorium_node_destroy(struct thorium_node *self);
int thorium_node_run(struct thorium_node *self);
void thorium_node_start_initial_actor(struct thorium_node *self);

int thorium_node_spawn_state(struct thorium_node *self, void *state,
                struct thorium_script *script);
int thorium_node_spawn(struct thorium_node *self, int script);
void thorium_node_send(struct thorium_node *self, struct thorium_message *message);
void thorium_node_send_with_transport(struct thorium_node *self, struct thorium_message *message);

int thorium_node_generate_name(struct thorium_node *self);

int thorium_node_actor_node(struct thorium_node *self, int name);
int thorium_node_actor_index(struct thorium_node *self, int name);
struct thorium_actor *thorium_node_get_actor_from_name(struct thorium_node *self,
                int name);

int thorium_node_name(struct thorium_node *self);
int thorium_node_nodes(struct thorium_node *self);
int thorium_node_actors(struct thorium_node *self);

void thorium_node_set_supervisor(struct thorium_node *self, int name, int supervisor);

void thorium_node_run_loop(struct thorium_node *self);

void thorium_node_send_message(struct thorium_node *self);
void thorium_node_notify_death(struct thorium_node *self, struct thorium_actor *actor);

void thorium_node_inject_message_in_worker_pool(struct thorium_node *self, struct thorium_message *message);
int thorium_node_pull(struct thorium_node *self, struct thorium_message *message);

int thorium_node_worker_count(struct thorium_node *self);
int thorium_node_thread_count(struct thorium_node *self);

int thorium_node_argc(struct thorium_node *self);
char **thorium_node_argv(struct thorium_node *self);
void *thorium_node_main(void *node1);
int thorium_node_running(struct thorium_node *self);
void thorium_node_start_send_thread(struct thorium_node *self);
int thorium_node_threads_from_string(struct thorium_node *self,
                char *required_threads, int index);

void thorium_node_add_script(struct thorium_node *self, int name, struct thorium_script *script);
struct thorium_script *thorium_node_find_script(struct thorium_node *self, int identifier);
int thorium_node_has_script(struct thorium_node *self, struct thorium_script *script);

void thorium_node_send_to_node(struct thorium_node *self, int destination,
                struct thorium_message *message);
void thorium_node_send_to_node_empty(struct thorium_node *self, int destination, int tag);
int thorium_node_receive_system(struct thorium_node *self, struct thorium_message *message);
void thorium_node_dispatch_message(struct thorium_node *self, struct thorium_message *message);
void thorium_node_set_initial_actor(struct thorium_node *self, int node_name, int actor);
int thorium_node_allocate_actor_index(struct thorium_node *self);

void thorium_node_print_event_counters(struct thorium_node *self);
void thorium_node_print_counters(struct thorium_node *self);

void thorium_node_handle_signal(int signal);
void thorium_node_register_signal_handlers(struct thorium_node *self);

void thorium_node_print_structure(struct thorium_node *self, struct thorium_actor *actor);
int thorium_node_has_actor(struct thorium_node *self, int name);

struct thorium_worker_pool *thorium_node_get_worker_pool(struct thorium_node *self);

void thorium_node_toggle_debug_mode(struct thorium_node *self);

void thorium_node_reset_actor_counters(struct thorium_node *self);

int64_t thorium_node_get_counter(struct thorium_node *self, int counter);
void thorium_node_test_requests(struct thorium_node *self);

void thorium_node_free_worker_buffer(struct thorium_node *self,
                struct thorium_worker_buffer *worker_buffer);

void thorium_node_send_to_actor(struct thorium_node *self, int name, struct thorium_message *message);
void thorium_node_check_efficiency(struct thorium_node *self);
int thorium_node_send_system(struct thorium_node *self, struct thorium_message *message);

void thorium_node_do_message_triage(struct thorium_node *self);
void thorium_node_recycle_message(struct thorium_node *self, struct thorium_message *message);

void thorium_node_prepare_received_message(struct thorium_node *self, struct thorium_message *message);
void thorium_node_resolve(struct thorium_node *self, struct thorium_message *message);

/*
 * Generate an actor name.
 */
int thorium_node_generate_random_name(struct thorium_node *self,
                int minimal_value, int maximum_value);

struct bsal_memory_pool *thorium_node_inbound_memory_pool(struct thorium_node *self);

void thorium_node_examine(struct thorium_node *self);
void thorium_node_inject_outbound_buffer(struct thorium_node *self, struct thorium_worker_buffer *worker_buffer);

#endif
