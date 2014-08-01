
#include "hello.h"

#include <biosal.h>

int main(int argc, char **argv)
{
    struct bsal_node node;

    bsal_node_init(&node, &argc, &argv);
    bsal_node_add_script(&node, HELLO_SCRIPT, &hello_script);
    bsal_node_spawn(&node, HELLO_SCRIPT);
    bsal_node_run(&node);
    bsal_node_destroy(&node);

    return 0;
}
