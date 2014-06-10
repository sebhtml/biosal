
#include <system/packer.h>

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

    buffer = malloc(8);

    struct bsal_packer packer;

    bsal_packer_init(&packer, BSAL_PACKER_OPERATION_PACK, buffer);
    bsal_packer_work(&packer, &expected_value, sizeof(expected_value));
    bsal_packer_destroy(&packer);

    bsal_packer_init(&packer, BSAL_PACKER_OPERATION_UNPACK, buffer);

    printf("DEBUG unpacking in test\n");
    bsal_packer_work(&packer, &actual_value, sizeof(actual_value));
    bsal_packer_destroy(&packer);

    printf("DEBUG actual_value %d expected_value %d\n",
                    actual_value, expected_value);

    TEST_INT_EQUALS(actual_value, expected_value);

    free(buffer);

    END_TESTS();

    return 0;
}
