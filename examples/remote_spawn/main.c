
#include "table.h"

#include <biosal.h>

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    struct bsal_node node;

    bsal_node_init(&node, &argc, &argv);
    bsal_node_add_script(&node, TABLE_SCRIPT, &table_script);
    bsal_node_spawn(&node, TABLE_SCRIPT);
    bsal_node_run(&node);
    bsal_node_destroy(&node);

    return 0;
}
