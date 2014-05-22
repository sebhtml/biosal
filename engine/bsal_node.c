
#include "bsal_node.h"

void bsal_node_spawn_actor(struct bsal_node *node, struct bsal_actor *actor)
{

}

void bsal_node_construct(struct bsal_node *node, int rank, int ranks, int threads)
{
    node->rank = rank;
    node->ranks = ranks;
    node->threads = threads;
}

void bsal_node_destruct(struct bsal_node *node)
{
}
