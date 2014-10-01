
#include <core/system/packer.h>
#include <core/system/memory.h>

#include <stdio.h>
#include <stdlib.h>

#include "test.h"

int main(int argc, char **argv)
{
    BEGIN_TESTS();

    int expected_value;
    int actual_value;

    void *buffer;

    expected_value = 42;
    actual_value = 100;

    buffer = core_memory_allocate(sizeof(expected_value), -1);

    struct core_packer packer;

    core_packer_init(&packer, CORE_PACKER_OPERATION_PACK, buffer);
    core_packer_process(&packer, &expected_value, sizeof(expected_value));
    core_packer_destroy(&packer);

    core_packer_init(&packer, CORE_PACKER_OPERATION_UNPACK, buffer);

    /*
    printf("DEBUG unpacking in test\n");
    */
    core_packer_process(&packer, &actual_value, sizeof(actual_value));
    core_packer_destroy(&packer);

    /*
    printf("DEBUG actual_value %d expected_value %d\n",
                    actual_value, expected_value);
                    */

    TEST_INT_EQUALS(actual_value, expected_value);

    core_memory_free(buffer, -1);

    END_TESTS();

    return 0;
}
