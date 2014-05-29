
#include <structures/hash_table_group.h>

#include "test.h"

int main(int argc, char **argv)
{
    struct bsal_hash_table_group group;
    int key_size;
    int value_size;

    key_size = 8;
    value_size = 48;

    BEGIN_TESTS();

    bsal_hash_table_group_init(&group, 64, key_size, value_size);

    TEST_POINTER_EQUALS(bsal_hash_table_group_get(&group, 0, key_size, value_size), NULL);
    TEST_POINTER_NOT_EQUALS(bsal_hash_table_group_add(&group, 0, key_size, value_size), NULL);

    bsal_hash_table_group_delete(&group, 0);
    TEST_POINTER_EQUALS(bsal_hash_table_group_get(&group, 0, key_size, value_size), NULL);

    bsal_hash_table_group_destroy(&group);

    END_TESTS();

    return 0;
}
