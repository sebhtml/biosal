
#include "reader.h"

#include <biosal.h>

int main(int argc, char **argv)
{
    return bsal_thorium_engine_boot_initial_actor(&argc, &argv, READER_SCRIPT, &reader_script);
}
