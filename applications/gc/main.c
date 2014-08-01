
#include "gc_ratio_calculator.h"

struct bsal_script gc_ratio_calculator_script;

int main(int argc, char *argv[])
{
    struct bsal_node node;

    bsal_node_init(&node, &argc, &argv);
    bsal_node_add_script(&node, GC_RATIO_CALCULATOR_SCRIPT, &gc_ratio_calculator_script);
    bsal_node_spawn(&node, GC_RATIO_CALCULATOR_SCRIPT);
    bsal_node_run(&node);
    bsal_node_destroy(&node);

    return 0;
}
