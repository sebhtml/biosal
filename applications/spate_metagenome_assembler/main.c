
#include "spate.h"

#include <biosal.h>

int main(int argc, char **argv)
{
    return bsal_thorium_engine_boot_initial_actor(&argc, &argv, SPATE_SCRIPT, &spate_script);
}
