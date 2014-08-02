
#include "mock.h"

#include <biosal.h>

/*
 * spawn one mock actor on the node and start
 * the node
 */
int main(int argc, char **argv)
{
    return bsal_thorium_engine_boot_initial_actor(&argc, &argv, MOCK_SCRIPT, &mock_script);
}
