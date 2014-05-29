
#include <structures/hash_table.h>

#include "test.h"

int main(int argc, char **argv)
{
    int i;
    struct bsal_hash_table table;
    int key_size;
    int value_size;
    uint64_t buckets;
    uint64_t key;

    BEGIN_TESTS();

    {
        buckets = 1048576;
        key = 1234;
        key_size = sizeof(key);
        value_size = 48;

        bsal_hash_table_init(&table, buckets, key_size, value_size);

        TEST_POINTER_EQUALS(bsal_hash_table_get(&table, &key), NULL);
        TEST_POINTER_NOT_EQUALS(bsal_hash_table_add(&table, &key), NULL);

        bsal_hash_table_delete(&table, &key);
        TEST_POINTER_EQUALS(bsal_hash_table_get(&table, &key), NULL);

        bsal_hash_table_destroy(&table);
    }

    {
        buckets = 4048;
        key = 1234;
        key_size = sizeof(key);
        value_size = 48;

        bsal_hash_table_init(&table, buckets, key_size, value_size);

        TEST_POINTER_EQUALS(bsal_hash_table_get(&table, &key), NULL);

        for (i = 0; i < 20; i++) {
            key = i;
            TEST_POINTER_NOT_EQUALS(bsal_hash_table_add(&table, &key), NULL);
        }

        bsal_hash_table_destroy(&table);
    }

    {
        /*printf("TEST OMEGA\n");*/

        buckets = 4041;
        key = 1234;
        key_size = sizeof(key);
        value_size = 48;

        bsal_hash_table_init(&table, buckets, key_size, value_size);

        /* init will allocate at least @buckets buckets */
        buckets = bsal_hash_table_buckets(&table);

        TEST_POINTER_EQUALS(bsal_hash_table_get(&table, &key), NULL);

        for (i = 0; i < buckets; i++) {
            key = i;

            TEST_INT_EQUALS(bsal_hash_table_elements(&table), i);
            /*printf("before adding %i\n", i);*/
            TEST_POINTER_NOT_EQUALS(bsal_hash_table_add(&table, &key), NULL);
            /*printf("elements after adding %i : %i\n", i,
                            bsal_hash_table_elements(&table)); */
            TEST_INT_EQUALS(bsal_hash_table_elements(&table), i + 1);
        }

        key = 9999;
        /* try to add something to a full table */
        TEST_POINTER_EQUALS(bsal_hash_table_add(&table, &key), NULL);
        TEST_INT_EQUALS(bsal_hash_table_elements(&table), buckets);

        bsal_hash_table_destroy(&table);
    }



    END_TESTS();

    return 0;
}
