
#include "sender.h"

#include <biosal.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    struct sender actor;
    struct bsal_node node;
    int threads;

    threads = 10;

    bsal_node_init(&node, threads, &argc, &argv);
    bsal_node_spawn(&node, &actor, &sender_vtable);
    bsal_node_start(&node);
    bsal_node_destroy(&node);

    return 0;
}
