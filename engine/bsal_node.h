
#include "bsal_actor.h"

struct bsal_node {
	int rank;
	int ranks;
	int threads;

	struct bsal_actor *actors;
};

void bsal_node_construct(struct bsal_node *node, int rank, int ranks, int threads);
void bsal_node_destruct(struct bsal_node *node);
void bsal_node_spawn_actor(struct bsal_node *node, struct bsal_actor *actor);
