
#ifndef BSAL_NODE_H
#define BSAL_NODE_H

#include "actor.h"
#include "work.h"
#include "transport.h"
#include "worker_pool.h"

#include <structures/vector.h>
#include <structures/queue.h>
#include <structures/dynamic_hash_table.h>

#include <system/lock.h>
#include <system/counter.h>

#include <pthread.h>

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
#define BSAL_NODE_ENABLE_LOAD_REPORTING
#define BSAL_NODE_LOAD_PERIOD 5

/*
 * Enable a check to avoid calling MPI probing function
 * when running on one single node.
 */
/*
#define BSAL_NODE_CHECK_MPI
*/

struct bsal_script;

/*
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
    struct bsal_dynamic_hash_table actor_names;
    struct bsal_vector initial_actors;
    int received_initial_actors;
    int ready;
    char print_structure;

    int started;

    struct bsal_map scripts;
    int available_scripts;
    int maximum_scripts;

#ifdef BSAL_NODE_CHECK_MPI
    int use_mpi;
#endif

    pthread_t thread;
    struct bsal_transport transport;
    struct bsal_lock spawn_and_death_lock;
    struct bsal_lock script_lock;

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
pthread_t *bsal_node_thread(struct bsal_node *node);
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

#endif
