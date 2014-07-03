
#include <core/structures/dynamic_hash_table.h>
#include <core/structures/dynamic_hash_table_iterator.h>

#include "test.h"

int main(int argc, char **argv)
{
    BEGIN_TESTS();

    {
        struct bsal_dynamic_hash_table table;
        uint64_t i;
        uint64_t elements;

        elements = 900;
        bsal_dynamic_hash_table_init(&table, 1140, sizeof(int), 0);

        for (i = 0; i < elements; i++) {
            /*
            printf("insert %d\n", i);

            printf("before actual %d expected %d\n", (int)bsal_dynamic_hash_table_size(&table), i);
            */
            TEST_UINT64_T_EQUALS(bsal_dynamic_hash_table_size(&table), i);

            bsal_dynamic_hash_table_add(&table, &i);

            /*
            printf("after1 actual %d expected %d\n", (int)bsal_dynamic_hash_table_size(&table), i + 1);
            */
            TEST_UINT64_T_EQUALS(bsal_dynamic_hash_table_size(&table), i + 1);
            /*
            printf("after2 actual %d expected %d\n", (int)bsal_dynamic_hash_table_size(&table), i + 1);
            */
            TEST_UINT64_T_EQUALS(bsal_dynamic_hash_table_size(&table), i + 1);
            /*
            printf("after3 actual %d expected %d\n", (int)bsal_dynamic_hash_table_size(&table), i + 1);
            */
            TEST_UINT64_T_EQUALS(bsal_dynamic_hash_table_size(&table), i + 1);
            /*
            printf("after4 actual %d expected %d\n", (int)bsal_dynamic_hash_table_size(&table), i + 1);
            */
            TEST_UINT64_T_EQUALS(bsal_dynamic_hash_table_size(&table), i + 1);
            /*
            printf("after5 actual %d expected %d\n", (int)bsal_dynamic_hash_table_size(&table), i + 1);
            */
            TEST_UINT64_T_EQUALS(bsal_dynamic_hash_table_size(&table), i + 1);
            /*
            printf("after6 actual %d expected %d\n", (int)bsal_dynamic_hash_table_size(&table), i + 1);
            */
        }

        bsal_dynamic_hash_table_destroy(&table);
    }

    {
        struct bsal_dynamic_hash_table table;
        struct bsal_dynamic_hash_table_iterator iterator;
        int key_size;
        int value_size;
        uint64_t buckets;
        uint64_t i;
        int j;
        uint64_t inserted;
        int key;
        int *key_bucket;
        int *value_bucket;
        int found;

        key_size = sizeof(int);
        value_size = sizeof(int);
        buckets = 4;
        inserted = 0;

        bsal_dynamic_hash_table_init(&table, buckets, key_size, value_size);

        for (i = 0; i < buckets; i++) {
            key = inserted;
            value_bucket = bsal_dynamic_hash_table_add(&table, &key);
            inserted++;

            TEST_POINTER_NOT_EQUALS(value_bucket, NULL);

            *value_bucket = i;

            TEST_UINT64_T_EQUALS(bsal_dynamic_hash_table_size(&table), inserted);

            value_bucket = bsal_dynamic_hash_table_get(&table, &key);

            TEST_POINTER_NOT_EQUALS(value_bucket, NULL);
        }

        key = inserted;
        value_bucket = bsal_dynamic_hash_table_add(&table, &key);
        inserted++;

        for (j = 0; j < 1000; j++) {
            for (i = 0; i < buckets; i++) {
                key = inserted;
                value_bucket = bsal_dynamic_hash_table_add(&table, &key);
                inserted++;

                TEST_POINTER_NOT_EQUALS(value_bucket, NULL);

                *value_bucket = i;

                /*
                printf("DEBUG actual %d expected %d\n",
                                (int)bsal_dynamic_hash_table_size(&table), inserted);
                                */

                TEST_UINT64_T_EQUALS(bsal_dynamic_hash_table_size(&table), inserted);

                value_bucket = bsal_dynamic_hash_table_get(&table, &key);

                TEST_POINTER_NOT_EQUALS(value_bucket, NULL);
            }
        }

        key = 9999;
        value_bucket = bsal_dynamic_hash_table_add(&table, &key);
        *value_bucket = 8888;

        bsal_dynamic_hash_table_iterator_init(&iterator, &table);

        i = 0;
        found = 0;

        while (bsal_dynamic_hash_table_iterator_has_next(&iterator)) {
            bsal_dynamic_hash_table_iterator_next(&iterator, (void **)&key_bucket,
                            (void **)&value_bucket);

            if (*key_bucket == 9999 && *value_bucket == 8888) {
                found = 1;
            }
            i++;
        }

        TEST_UINT64_T_EQUALS(i, bsal_dynamic_hash_table_size(&table));
        TEST_INT_EQUALS(found, 1);

        bsal_dynamic_hash_table_iterator_destroy(&iterator);

        bsal_dynamic_hash_table_destroy(&table);
    }

    {
        struct bsal_dynamic_hash_table table;
        int key;
        int *value;

        /*
        printf("-------------------\n");
        printf("DEBUG TEST-alpha-89\n");
        */

        bsal_dynamic_hash_table_init(&table, 8, sizeof(int), sizeof(int));

        for (key = 0; key < 1000000; key++) {

/*
            printf("DEBUG key %d\n", key);
            */
            bsal_dynamic_hash_table_add(&table, &key);

            value = (int *)bsal_dynamic_hash_table_get(&table, &key);

            TEST_POINTER_NOT_EQUALS(value, NULL);

            *value = key;
        }

        bsal_dynamic_hash_table_destroy(&table);
    }

    {
        struct bsal_dynamic_hash_table table;
        uint64_t i;

        bsal_dynamic_hash_table_init(&table, 2, sizeof(int), sizeof(int));

        for (i = 0; i < 999; i++) {

            bsal_dynamic_hash_table_add(&table, &i);

            TEST_UINT64_T_EQUALS(bsal_dynamic_hash_table_size(&table), i + 1);
        }

        bsal_dynamic_hash_table_destroy(&table);
    }

    {
        struct bsal_dynamic_hash_table table;
        uint64_t i;
        void *bucket;

        bsal_dynamic_hash_table_init(&table, 2, sizeof(int), 128);

        for (i = 0; i < 999; i++) {

            bsal_dynamic_hash_table_add(&table, &i);

            TEST_UINT64_T_EQUALS(bsal_dynamic_hash_table_size(&table), i + 1);
        }

        for (i = 0; i < 999; i++) {
            bucket = bsal_dynamic_hash_table_get(&table, &i);

            TEST_POINTER_NOT_EQUALS(bucket, NULL);
        }

        bsal_dynamic_hash_table_destroy(&table);

    }

    END_TESTS();

    return 0;
}
