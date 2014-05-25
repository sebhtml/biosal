
#ifndef _BSAL_NODE_H
#define _BSAL_NODE_H

#include "bsal_actor.h"
#include "bsal_work.h"
#include "bsal_thread.h"

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
    int rank;
    int size;
    int threads;

    struct bsal_actor *actors;
    int actor_count;
    int dead_actors;
    int alive_actors;

    MPI_Comm comm;
    MPI_Datatype datatype;

    struct bsal_thread thread;

    pthread_mutex_t death_mutex;
};

void bsal_node_construct(struct bsal_node *node, int threads, int *argc, char ***argv);
void bsal_node_destruct(struct bsal_node *node);
void bsal_node_start(struct bsal_node *node);
int bsal_node_spawn(struct bsal_node *node, void *pointer,
                struct bsal_actor_vtable *vtable);

void bsal_node_send(struct bsal_node *node, struct bsal_message *message);

int bsal_node_assign_name(struct bsal_node *node);
int bsal_node_actor_rank(struct bsal_node *node, int name);
int bsal_node_actor_index(struct bsal_node *node, int rank, int name);
int bsal_node_rank(struct bsal_node *node);
int bsal_node_size(struct bsal_node *node);

void bsal_node_run(struct bsal_node *node);

void bsal_node_resolve(struct bsal_node *node, struct bsal_message *message);

void bsal_node_send_outbound_message(struct bsal_node *node, struct bsal_message *message);
void bsal_node_notify_death(struct bsal_node *node, struct bsal_actor *actor);

int bsal_node_receive(struct bsal_node *node, struct bsal_message *message);
void bsal_node_dispatch(struct bsal_node *node, struct bsal_message *message);
void bsal_node_assign_work(struct bsal_node *node, struct bsal_work *work);
int bsal_node_pull(struct bsal_node *node, struct bsal_message *message);

struct bsal_thread *bsal_node_select_thread_for_pull(struct bsal_node *node);

#endif
