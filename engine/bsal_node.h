
#include "bsal_actor.h"
#include <mpi.h>

struct bsal_node {
    int rank;
    int ranks;
    int threads;

    struct bsal_actor *actors;
    int actor_count;

	MPI_Comm comm;
};

void bsal_node_construct(struct bsal_node *node, int threads, int *argc, char ***argv);
void bsal_node_destruct(struct bsal_node *node);
void bsal_node_start(struct bsal_node *node);
void bsal_node_spawn_actor(struct bsal_node *node, struct bsal_actor *actor);
void bsal_node_send(struct bsal_node *node, int name, struct bsal_message *message);
int bsal_node_assign_name(struct bsal_node *node);
int bsal_node_get_index(struct bsal_node *node, int name);
