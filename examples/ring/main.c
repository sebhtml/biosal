
#include "sender.h"

#include <biosal.h>

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    struct bsal_node node;

    bsal_node_init(&node, &argc, &argv);

    bsal_node_add_script(&node, SENDER_SCRIPT, &sender_vtable);

    bsal_node_spawn(&node, SENDER_SCRIPT);
    bsal_node_start(&node);
    bsal_node_destroy(&node);

    return 0;
}
