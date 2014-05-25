
#include "mock.h"
#include <biosal.h>

/*
 * spawn one mock actor on the node and start
 * the node
 */
int main(int argc, char **argv)
{
    struct mock mock1;
    struct bsal_node node;
    int threads;

    threads = 4;
    bsal_node_construct(&node, threads, &argc, &argv);
    bsal_node_spawn(&node, &mock1, &mock_vtable);
    bsal_node_start(&node);

    bsal_node_destruct(&node);

    return 0;
}
