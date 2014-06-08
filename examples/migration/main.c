
#include "frame.h"

#include <biosal.h>

int main(int argc, char **argv)
{
    struct bsal_node node;

    bsal_node_init(&node, &argc, &argv);
    bsal_node_add_script(&node, FRAME_SCRIPT, &frame_script);
    bsal_node_spawn(&node, FRAME_SCRIPT);
    bsal_node_run(&node);
    bsal_node_destroy(&node);

    return 0;
}
