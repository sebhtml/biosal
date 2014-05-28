
#include <structures/hash_table.h>

#include "test.h"

int main(int argc, char **argv)
{
    int i;
    struct bsal_hash_table table;
    int key_size;
    int value_size;
    uint64_t buckets;
    int bucket_per_group;
    uint64_t key;

    BEGIN_TESTS();

    {
        buckets = 1048576;
        bucket_per_group = 64;
        key = 1234;
        key_size = sizeof(key);
        value_size = 48;

        bsal_hash_table_init(&table, buckets, bucket_per_group, key_size, value_size);

        TEST_POINTER_EQUALS(bsal_hash_table_get(&table, &key), NULL);
        TEST_POINTER_NOT_EQUALS(bsal_hash_table_add(&table, &key), NULL);

        bsal_hash_table_delete(&table, &key);
        TEST_POINTER_EQUALS(bsal_hash_table_get(&table, &key), NULL);

        bsal_hash_table_destroy(&table);
    }

    {
        buckets = 4048;
        bucket_per_group = 64;
        key = 1234;
        key_size = sizeof(key);
        value_size = 48;

        bsal_hash_table_init(&table, buckets, bucket_per_group, key_size, value_size);

        TEST_POINTER_EQUALS(bsal_hash_table_get(&table, &key), NULL);

        for (i = 0; i < 20; i++) {
            key = i;
            TEST_POINTER_NOT_EQUALS(bsal_hash_table_add(&table, &key), NULL);
        }

        bsal_hash_table_destroy(&table);
    }

    END_TESTS();

    return 0;
}
