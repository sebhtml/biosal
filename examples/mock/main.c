
#include "mock.h"
#include <biosal.h>

int main(int argc, char **argv)
{
    struct mock mock1;
    struct bsal_actor actor1;
    struct bsal_node node;
    int threads;

    threads = 4;
    bsal_node_construct(&node, threads, &argc, &argv);

    bsal_actor_construct(&actor1, &mock1, &mock_vtable);
    bsal_node_spawn(&node, &actor1);

    bsal_node_start(&node);

    bsal_node_destruct(&node);
    bsal_actor_destruct(&actor1);

    return 0;
}
