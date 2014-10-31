
#include "process.h"

#include <biosal.h>

int main(int argc, char **argv)
{
    return biosal_thorium_engine_boot_initial_actor(&argc, &argv, SCRIPT_BINOMIAL_TREE_PROCESS,
                    &process_script);
}
