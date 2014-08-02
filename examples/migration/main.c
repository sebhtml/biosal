
#include "frame.h"

#include <biosal.h>

int main(int argc, char **argv)
{
    return bsal_thorium_engine_boot_initial_actor(&argc, &argv, FRAME_SCRIPT, &frame_script);
}
