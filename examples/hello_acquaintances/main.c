
#include "hello_acq.h"

#include <biosal.h>

int main(int argc, char **argv)
{
    return biosal_thorium_engine_boot_initial_actor(&argc, &argv, SCRIPT_HELLO_ACQ, &hello_acq_script);
}
