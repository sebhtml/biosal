
#include <core/structures/hash_table.h>
#include <core/structures/hash_table_iterator.h>

#include "test.h"

int main(int argc, char **argv)
{
    uint64_t i;
    struct core_hash_table table;
    int key_size;
    int value_size;
    uint64_t buckets;
    uint64_t key;

    int *value;
    int actual_value;
    int expected_value;

    BEGIN_TESTS();

    {
        core_hash_table_init(&table, 900, sizeof(int), 0);

        for (i = 0; i < 500; i++) {
            TEST_UINT64_T_EQUALS(core_hash_table_size(&table), i);
            core_hash_table_add(&table, &i);
            TEST_UINT64_T_EQUALS(core_hash_table_size(&table), i + 1);
        }

        core_hash_table_destroy(&table);
    }

    {
        buckets = 1048576;
        key = 1234;
        key_size = sizeof(key);
        value_size = 48;

        core_hash_table_init(&table, buckets, key_size, value_size);

        TEST_POINTER_EQUALS(core_hash_table_get(&table, &key), NULL);
        TEST_POINTER_NOT_EQUALS(core_hash_table_add(&table, &key), NULL);

        core_hash_table_delete(&table, &key);
        TEST_POINTER_EQUALS(core_hash_table_get(&table, &key), NULL);

        core_hash_table_destroy(&table);
    }

    {
        buckets = 4048;
        key = 1234;
        key_size = sizeof(key);
        value_size = 48;

        core_hash_table_init(&table, buckets, key_size, value_size);

        TEST_POINTER_EQUALS(core_hash_table_get(&table, &key), NULL);

        for (i = 0; i < 20; i++) {
            key = i;
            TEST_POINTER_NOT_EQUALS(core_hash_table_add(&table, &key), NULL);
        }

        core_hash_table_destroy(&table);
    }

    {
        /*printf("TEST OMEGA\n");*/

        buckets = 4041;
        key = 1234;
        key_size = sizeof(key);
        value_size = 48;

        core_hash_table_init(&table, buckets, key_size, value_size);

        /* init will allocate at least @buckets buckets */
        buckets = core_hash_table_buckets(&table);

        TEST_POINTER_EQUALS(core_hash_table_get(&table, &key), NULL);

        for (i = 0; i < buckets; i++) {
            key = i;

            TEST_UINT64_T_EQUALS(core_hash_table_size(&table), i);
            /*printf("before adding %i\n", i);*/
            /*printf("add %i DEBUG ", i); */
            TEST_POINTER_NOT_EQUALS(core_hash_table_add(&table, &key), NULL);
            TEST_UINT64_T_EQUALS(core_hash_table_size(&table), i + 1);

            TEST_POINTER_NOT_EQUALS(core_hash_table_get(&table, &key), NULL);
            TEST_POINTER_NOT_EQUALS(core_hash_table_get(&table, &key), NULL);

            value = core_hash_table_get(&table, &key);
            expected_value = i * 2;
            *value = expected_value;

            value = core_hash_table_get(&table, &key);
            /*printf("expected %i actual %i\n", i * 2, *value);*/

            actual_value = *value;

            /*printf("actual_value %i\n", actual_value); */

            TEST_INT_EQUALS(actual_value, expected_value);
            actual_value++;
            expected_value++;

            if (i >= 21) {
                key = 21;
                TEST_POINTER_NOT_EQUALS(core_hash_table_get(&table, &key),
                                NULL);
            }
        }

        key = 9999;
        /* try to add something to a full table */
        TEST_POINTER_EQUALS(core_hash_table_add(&table, &key), NULL);
        TEST_UINT64_T_EQUALS(core_hash_table_size(&table), buckets);

        for (i = 0; i < buckets; i++) {

            if (i < 21) {
                key = 21;
                TEST_POINTER_NOT_EQUALS(core_hash_table_get(&table, &key),
                                NULL);
            }

            key = i;

            /*printf("test core_hash_table_get key %i value %p\n",
                            i, core_hash_table_get(&table, &key));*/
            TEST_POINTER_NOT_EQUALS(core_hash_table_get(&table, &key), NULL);

            /*printf("delete %i\n", i); */

            core_hash_table_delete(&table, &key);
            TEST_POINTER_EQUALS(core_hash_table_get(&table, &key), NULL);
            /*printf("get %i returns %p\n", i, core_hash_table_get(&table, &key)); */

            if (i < 21) {
                key = 21;
                TEST_POINTER_NOT_EQUALS(core_hash_table_get(&table, &key),
                                NULL);
            }
        }

        TEST_UINT64_T_EQUALS(core_hash_table_size(&table), 0);
        TEST_UINT64_T_EQUALS(core_hash_table_buckets(&table), buckets);

        core_hash_table_destroy(&table);
    }

    {
        struct core_hash_table table;
        int key;
        int value;
        int actual_value;
        int *bucket;

        core_hash_table_init(&table, 16, sizeof(int), sizeof(int));
        TEST_UINT64_T_EQUALS(core_hash_table_size(&table), 0);

        key = 1231243213;
        value = 1;
        actual_value = 1;

        core_hash_table_toggle_debug(&table);
        bucket = core_hash_table_add(&table, &key);
        TEST_POINTER_NOT_EQUALS(bucket, NULL);
        *bucket = value;

        bucket = core_hash_table_get(&table, &key);
        TEST_POINTER_NOT_EQUALS(bucket, NULL);
        value = *bucket;
        TEST_INT_EQUALS(value, actual_value);

        core_hash_table_destroy(&table);

    }

    {
        struct core_hash_table table;
        struct core_hash_table_iterator iterator;
        int *key_bucket;
        int *value_bucket;
        int i;

        core_hash_table_init(&table, 30000, sizeof(int), sizeof(int));
        TEST_UINT64_T_EQUALS(core_hash_table_size(&table), 0);

        for (i = 0; i < 1000; i++) {
            core_hash_table_add(&table, &i);

            value_bucket = core_hash_table_get(&table, &i);

            *value_bucket = i;
        }

        TEST_UINT64_T_EQUALS(core_hash_table_size(&table), 1000);

        core_hash_table_iterator_init(&iterator, &table);

        i = 0;

        while (core_hash_table_iterator_has_next(&iterator)) {

            core_hash_table_iterator_next(&iterator, (void **)&key_bucket,
                            (void **)&value_bucket);

            /*
            printf("DEBUG %d %d %d\n", i, *key_bucket, *value_bucket);
            */

            i++;
        }

        TEST_INT_EQUALS(i, 1000);

        core_hash_table_iterator_destroy(&iterator);

        core_hash_table_destroy(&table);
    }

    END_TESTS();

    return 0;
}
