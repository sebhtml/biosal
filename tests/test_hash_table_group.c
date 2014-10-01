
#include <core/structures/hash_table_group.h>
#include <core/structures/hash_table_group_iterator.h>

#include "test.h"

int main(int argc, char **argv)
{
    struct biosal_hash_table_group group;
    int key_size;
    int value_size;
    int buckets;
    int i;
    int *key;
    int *value;
    struct biosal_hash_table_group_iterator iterator;

    buckets = 64;
    key_size = sizeof(int);
    value_size = sizeof(int);

    BEGIN_TESTS();

    biosal_hash_table_group_init(&group, buckets, key_size, value_size, NULL, 1);

    TEST_POINTER_EQUALS(biosal_hash_table_group_get(&group, 0, key_size, value_size), NULL);
    TEST_POINTER_NOT_EQUALS(biosal_hash_table_group_add(&group, 0, key_size, value_size), NULL);

    biosal_hash_table_group_delete(&group, 0);
    TEST_POINTER_EQUALS(biosal_hash_table_group_get(&group, 0, key_size, value_size), NULL);

    biosal_hash_table_group_destroy(&group, NULL);

    {
        biosal_hash_table_group_init(&group, buckets, key_size, value_size, NULL, 1);

        for (i = 0; i < 10; i++) {
            value = biosal_hash_table_group_add(&group, i, key_size, value_size);
            key = biosal_hash_table_group_key(&group, i, key_size, value_size);
            *key = i;
            *value = i;
        }

        biosal_hash_table_group_iterator_init(&iterator, &group, buckets, key_size, value_size);

        i = 0;

        while (biosal_hash_table_group_iterator_has_next(&iterator)) {
            biosal_hash_table_group_iterator_next(&iterator, (void **)&key, (void **)&value);

            /*
            printf("DEBUG %d %d\n", *key, *value);
            */

            TEST_POINTER_NOT_EQUALS(key, NULL);
            TEST_POINTER_NOT_EQUALS(value, NULL);
            TEST_INT_EQUALS(*key, i);
            TEST_INT_EQUALS(*value, i);

            i++;
        }

        biosal_hash_table_group_iterator_destroy(&iterator);

        biosal_hash_table_group_destroy(&group, NULL);

    }

    END_TESTS();

    return 0;
}
