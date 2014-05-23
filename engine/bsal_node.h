
#include "bsal_actor.h"
#include <mpi.h>

struct bsal_node {
    int rank;
    int size;
    int threads;

    struct bsal_actor *actors;
    int actor_count;

	MPI_Comm comm;
};

void bsal_node_construct(struct bsal_node *node, int threads, int *argc, char ***argv);
void bsal_node_destruct(struct bsal_node *node);
void bsal_node_start(struct bsal_node *node);
void bsal_node_spawn(struct bsal_node *node, struct bsal_actor *actor);

void bsal_node_send(struct bsal_node *node, struct bsal_message *message);
void bsal_node_receive(struct bsal_node *node, struct bsal_message *message);
void bsal_node_send_here(struct bsal_node *node, struct bsal_message *message);
void bsal_node_send_elsewhere(struct bsal_node *node, struct bsal_message *message);

int bsal_node_assign_name(struct bsal_node *node);
int bsal_node_actor_rank(struct bsal_node *node, int name);
int bsal_node_actor_index(struct bsal_node *node, int rank, int name);
int bsal_node_rank(struct bsal_node *node);
int bsal_node_size(struct bsal_node *node);

