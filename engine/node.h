
#ifndef _BSAL_NODE_H
#define _BSAL_NODE_H

#include "actor.h"
#include "work.h"
#include "worker_pool.h"

#include <pthread.h>
#include <mpi.h>

struct bsal_actor_vtable;

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

    pthread_spinlock_t death_lock;
    pthread_spinlock_t spawn_lock;

    MPI_Comm comm;
    MPI_Datatype datatype;

    int rank;
    int nodes;

    int actor_count;
    int actor_capacity;
    int dead_actors;
    int alive_actors;

    int argc;
    char **argv;
};

void bsal_node_init(struct bsal_node *node, int threads, int *argc, char ***argv);
void bsal_node_destroy(struct bsal_node *node);
void bsal_node_start(struct bsal_node *node);
int bsal_node_spawn(struct bsal_node *node, void *pointer,
                struct bsal_actor_vtable *vtable);

void bsal_node_send(struct bsal_node *node, struct bsal_message *message);

int bsal_node_assign_name(struct bsal_node *node);

int bsal_node_actor_rank(struct bsal_node *node, int name);
int bsal_node_actor_index(struct bsal_node *node, int rank, int name);
struct bsal_actor *bsal_node_get_actor_from_name(struct bsal_node *node,
                int name);

int bsal_node_rank(struct bsal_node *node);
int bsal_node_nodes(struct bsal_node *node);
void bsal_node_set_supervisor(struct bsal_node *node, int name, int supervisor);

void bsal_node_run(struct bsal_node *node);

/* MPI ranks are set with bsal_node_resolve */
void bsal_node_resolve(struct bsal_node *node, struct bsal_message *message);

void bsal_node_send_outbound_message(struct bsal_node *node, struct bsal_message *message);
void bsal_node_notify_death(struct bsal_node *node, struct bsal_actor *actor);

int bsal_node_receive(struct bsal_node *node, struct bsal_message *message);
void bsal_node_create_work(struct bsal_node *node, struct bsal_message *message);
int bsal_node_pull(struct bsal_node *node, struct bsal_message *message);
int bsal_node_workers(struct bsal_node *node);

int bsal_node_argc(struct bsal_node *node);
char **bsal_node_argv(struct bsal_node *node);

#endif
