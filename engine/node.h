
#ifndef _BSAL_NODE_H
#define _BSAL_NODE_H

#include "actor.h"
#include "work.h"
#include "worker_pool.h"
#include "active_buffer.h"

#include <structures/fifo.h>

#include <pthread.h>
#include <mpi.h>

struct bsal_script;

/*
 * - message reception: one fifo per thread
 * - the fifo has volatile variables for its heads
 * - fifo is implemented as linked list of arrays
 *
 * - message send process: one fifo per thread
 * - for states,  use OR (|)
 * - use actor affinity in implementation
 *
 */
struct bsal_node {
    struct bsal_actor *actors;
    struct bsal_worker_pool worker_pool;

    struct bsal_script **scripts;
    int available_scripts;
    int maximum_scripts;

    pthread_t thread;

    pthread_spinlock_t death_lock;
    pthread_spinlock_t spawn_lock;
    pthread_spinlock_t script_lock;

    struct bsal_fifo active_buffers;

    MPI_Comm comm;
    MPI_Datatype datatype;
    int provided;

    int name;
    int nodes;
    int threads;

    int worker_in_main_thread;
    int send_in_thread;
    int workers_in_threads;

    int actor_count;
    int actor_capacity;
    int dead_actors;
    int alive_actors;

    int argc;
    char **argv;

    int debug;

    uint64_t counter_messages_sent_to_self;
    uint64_t counter_messages_received_from_other;
    uint64_t counter_messages_sent_to_other;
};

void bsal_node_init(struct bsal_node *node, int *argc, char ***argv);
void bsal_node_destroy(struct bsal_node *node);
void bsal_node_run(struct bsal_node *node);
int bsal_node_spawn_state(struct bsal_node *node, void *state,
                struct bsal_script *script);
int bsal_node_spawn(struct bsal_node *node, int script);
void bsal_node_send(struct bsal_node *node, struct bsal_message *message);

int bsal_node_assign_name(struct bsal_node *node);

int bsal_node_actor_node(struct bsal_node *node, int name);
int bsal_node_actor_index(struct bsal_node *node, int node_name, int name);
struct bsal_actor *bsal_node_get_actor_from_name(struct bsal_node *node,
                int name);

int bsal_node_name(struct bsal_node *node);
int bsal_node_nodes(struct bsal_node *node);
void bsal_node_set_supervisor(struct bsal_node *node, int name, int supervisor);

void bsal_node_run_loop(struct bsal_node *node);

/* MPI ranks are set with bsal_node_resolve */
void bsal_node_resolve(struct bsal_node *node, struct bsal_message *message);

void bsal_node_send_message(struct bsal_node *node);
void bsal_node_send_outbound_message(struct bsal_node *node, struct bsal_message *message);
void bsal_node_notify_death(struct bsal_node *node, struct bsal_actor *actor);

int bsal_node_receive(struct bsal_node *node, struct bsal_message *message);
void bsal_node_create_work(struct bsal_node *node, struct bsal_message *message);
int bsal_node_pull(struct bsal_node *node, struct bsal_message *message);

int bsal_node_workers(struct bsal_node *node);
int bsal_node_threads(struct bsal_node *node);

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
uint64_t bsal_node_get_counter(struct bsal_node *node, int counter);
void bsal_node_increment_counter(struct bsal_node *node, int counter);

void bsal_node_test_requests(struct bsal_node *node);

#endif
