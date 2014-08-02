
#include "hello.h"

#include <biosal.h>

int main(int argc, char **argv)
{
    return bsal_thorium_boot_initial_actor(&argc, &argv, HELLO_SCRIPT, &hello_script);
}
