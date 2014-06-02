
#include "reader.h"

#include <biosal.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    struct bsal_node node;

    bsal_node_init(&node, &argc, &argv);

    bsal_node_add_script(&node, READER_SCRIPT, &reader_script);
    bsal_node_add_script(&node, BSAL_INPUT_ACTOR_SCRIPT,
                    &bsal_input_script);

    bsal_node_spawn(&node, READER_SCRIPT);
    bsal_node_run(&node);
    bsal_node_destroy(&node);

    return 0;
}
