
#include "mock.h"
#include "buddy.h"

#include <biosal.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * spawn one mock actor on the node and start
 * the node
 */
int main(int argc, char **argv)
{
    struct bsal_node node;

    bsal_node_init(&node, &argc, &argv);

    bsal_node_add_script(&node, MOCK_SCRIPT, &mock_script);

    bsal_node_spawn(&node, MOCK_SCRIPT);
    bsal_node_run(&node);
    bsal_node_destroy(&node);

    return 0;
}
