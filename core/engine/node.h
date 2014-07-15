
#ifndef BSAL_NODE_H
#define BSAL_NODE_H

#include "actor.h"
#include "transport.h"
#include "worker_pool.h"

#include <core/structures/vector.h>
#include <core/structures/queue.h>
#include <core/structures/map.h>

#include <core/system/lock.h>
#include <core/system/counter.h>
#include <core/system/memory_pool.h>

/*
 * \see http://pubs.opengroup.org/onlinepubs/009696699/basedefs/signal.h.html
 */
#include <signal.h>

#define BSAL_NODE_ADD_INITIAL_ACTOR 0x00002438
#define BSAL_NODE_ADD_INITIAL_ACTORS 0x00004c19
#define BSAL_NODE_ADD_INITIAL_ACTORS_REPLY 0x00003ad3
#define BSAL_NODE_START 0x0000082c

/*
 * Compilation options:
 */

/*
 * Enable load reporting with -print-load and memory reporting with
 * -print-memory-usage
 */
/*
 */
#define BSAL_NODE_ENABLE_INSTRUMENTATION
#define BSAL_NODE_LOAD_PERIOD 5

/*
 * Enable a check to avoid calling MPI probing function
 * when running on one single node.
 */
/*
*/
#define BSAL_NODE_CHECK_MPI

struct bsal_script;

/*
 * This is the Thorium distributed actor engine developed
 * at Argonne National Laboratory.
 *
 * BIOSAL applications are developed by creating actors
 * and by running them in Thorium.
 *
 * Thorium has these components:
 *
 * - Runtime node (struct bsal_node)
 * - Actor scheduler (struct bsal_scheduler)
 * - Actor (struct bsal_actor)
 * - Message (struct bsal_message)
 * - Worker pool (struct bsal_worker_pool)
 * - Worker (struct bsal_worker)
 * - Actor script (struct bsal_script)
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
struct bsal_node {
    struct bsal_vector actors;
    struct bsal_worker_pool worker_pool;
    struct bsal_map actor_names;
    struct bsal_vector initial_actors;
    int received_initial_actors;
    int ready;
    char print_structure;

#ifdef BSAL_NODE_USE_MESSAGE_RECYCLING
    struct bsal_ring_queue outbound_buffers;
#endif

    int started;

    struct bsal_map scripts;
    int available_scripts;
    int maximum_scripts;

#ifdef BSAL_NODE_CHECK_MPI
    int use_mpi;
#endif

    struct bsal_thread thread;
    struct bsal_transport transport;
    struct bsal_lock spawn_and_death_lock;
    struct bsal_lock script_lock;

    struct bsal_memory_pool actor_memory_pool;
    struct bsal_memory_pool inbound_message_memory_pool;
    struct bsal_memory_pool node_message_memory_pool;

    struct bsal_queue dead_indices;

    int provided;

    int name;
    int nodes;
    int threads;

    int worker_in_main_thread;
    int send_in_thread;
    int workers_in_threads;

    int dead_actors;
    int alive_actors;

    int argc;
    char **argv;

    int debug;
    int debug_mode;

    struct bsal_counter counter;

    /*
     * Requires -D_POSIX_C_SOURCE=200112L
     */
    struct sigaction action;

    time_t start_time;
    time_t last_report_time;
    char print_load;
    char print_memory_usage;
    char print_counters;
};

void bsal_node_init(struct bsal_node *node, int *argc, char ***argv);
void bsal_node_destroy(struct bsal_node *node);
void bsal_node_run(struct bsal_node *node);
void bsal_node_start_initial_actor(struct bsal_node *node);

int bsal_node_spawn_state(struct bsal_node *node, void *state,
                struct bsal_script *script);
int bsal_node_spawn(struct bsal_node *node, int script);
void bsal_node_send(struct bsal_node *node, struct bsal_message *message);

int bsal_node_generate_name(struct bsal_node *node);

int bsal_node_actor_node(struct bsal_node *node, int name);
int bsal_node_actor_index(struct bsal_node *node, int name);
struct bsal_actor *bsal_node_get_actor_from_name(struct bsal_node *node,
                int name);

int bsal_node_name(struct bsal_node *node);
int bsal_node_nodes(struct bsal_node *node);
int bsal_node_actors(struct bsal_node *node);

void bsal_node_set_supervisor(struct bsal_node *node, int name, int supervisor);

void bsal_node_run_loop(struct bsal_node *node);

void bsal_node_send_message(struct bsal_node *node);
void bsal_node_notify_death(struct bsal_node *node, struct bsal_actor *actor);

void bsal_node_create_work(struct bsal_node *node, struct bsal_message *message);
int bsal_node_pull(struct bsal_node *node, struct bsal_message *message);

int bsal_node_worker_count(struct bsal_node *node);
int bsal_node_thread_count(struct bsal_node *node);

int bsal_node_argc(struct bsal_node *node);
char **bsal_node_argv(struct bsal_node *node);
void *bsal_node_main(void *node1);
int bsal_node_running(struct bsal_node *node);
void bsal_node_start_send_thread(struct bsal_node *node);
int bsal_node_threads_from_string(struct bsal_node *node,
                char *required_threads, int index);

void bsal_node_add_script(struct bsal_node *node, int name, struct bsal_script *script);
struct bsal_script *bsal_node_find_script(struct bsal_node *node, int name);
int bsal_node_has_script(struct bsal_node *node, struct bsal_script *script);

void bsal_node_send_to_node(struct bsal_node *node, int destination,
                struct bsal_message *message);
void bsal_node_send_to_node_empty(struct bsal_node *node, int destination, int tag);
int bsal_node_receive_system(struct bsal_node *node, struct bsal_message *message);
void bsal_node_dispatch_message(struct bsal_node *node, struct bsal_message *message);
void bsal_node_set_initial_actor(struct bsal_node *node, int node_name, int actor);
int bsal_node_allocate_actor_index(struct bsal_node *node);

void bsal_node_print_event_counters(struct bsal_node *node);
void bsal_node_print_counters(struct bsal_node *node);

void bsal_node_handle_signal(int signal);
void bsal_node_register_signal_handlers(struct bsal_node *self);

void bsal_node_print_structure(struct bsal_node *node, struct bsal_actor *actor);
int bsal_node_has_actor(struct bsal_node *self, int name);

struct bsal_worker_pool *bsal_node_get_worker_pool(struct bsal_node *self);

void bsal_node_toggle_debug_mode(struct bsal_node *self);

void bsal_node_reset_actor_counters(struct bsal_node *node);

int64_t bsal_node_get_counter(struct bsal_node *node, int counter);
void bsal_node_test_requests(struct bsal_node *node);

void bsal_node_free_active_buffer(struct bsal_node *node,
                struct bsal_active_buffer *active_buffer);

#endif
