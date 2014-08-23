
#include "ring.h"

#include <biosal.h>

int main(int argc, char **argv)
{
    return bsal_thorium_engine_boot_initial_actor(&argc, &argv, SCRIPT_RING, &ring_script);
}
