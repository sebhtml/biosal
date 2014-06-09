
#include <structures/dynamic_hash_table.h>

#include "test.h"

int main(int argc, char **argv)
{
    BEGIN_TESTS();

    {
        struct bsal_dynamic_hash_table table;
        int key_size;
        int value_size;
        int buckets;
        int i;
        int j;
        int *value_bucket;
        int inserted;
        int key;

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

            TEST_INT_EQUALS(bsal_dynamic_hash_table_size(&table), inserted);

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

                TEST_INT_EQUALS(bsal_dynamic_hash_table_size(&table), inserted);

                value_bucket = bsal_dynamic_hash_table_get(&table, &key);

                TEST_POINTER_NOT_EQUALS(value_bucket, NULL);
            }
        }

        bsal_dynamic_hash_table_destroy(&table);
    }

    END_TESTS();

    return 0;
}
