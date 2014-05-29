
#include "reader.h"

#include <biosal.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    struct reader actor;
    struct bsal_node node;
    int threads;

    threads = 1;
    bsal_node_init(&node, threads, &argc, &argv);
    bsal_node_spawn(&node, &actor, &reader_vtable);
    bsal_node_start(&node);
    bsal_node_destroy(&node);

    return 0;
}
