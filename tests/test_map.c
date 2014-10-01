
#include <genomics/data/dna_kmer.h>
#include <core/structures/map.h>
#include <core/structures/map_iterator.h>
#include <core/system/memory.h>

#include "test.h"

#include <string.h>
#include <inttypes.h>

int main(int argc, char **argv)
{
    BEGIN_TESTS();

    {
        struct core_map table;
        struct core_map_iterator iterator;
        int key_size;
        int value_size;
        uint64_t i;
        int j;
        uint64_t buckets;
        uint64_t inserted;
        int key;
        int *key_bucket;
        int *value_bucket;
        int found;

        key_size = sizeof(int);
        value_size = sizeof(int);
        buckets = 500;
        inserted = 0;

        core_map_init(&table, key_size, value_size);

        for (i = 0; i < buckets; i++) {
            key = inserted;
            value_bucket = core_map_add(&table, &key);
            inserted++;

            TEST_POINTER_NOT_EQUALS(value_bucket, NULL);

            *value_bucket = i;

            TEST_UINT64_T_EQUALS(core_map_size(&table), inserted);

            value_bucket = core_map_get(&table, &key);

            TEST_POINTER_NOT_EQUALS(value_bucket, NULL);
        }

        key = inserted;
        value_bucket = core_map_add(&table, &key);
        inserted++;

        for (j = 0; j < 1000; j++) {
            for (i = 0; i < buckets; i++) {
                key = inserted;
                value_bucket = core_map_add(&table, &key);
                inserted++;

                TEST_POINTER_NOT_EQUALS(value_bucket, NULL);

                *value_bucket = i;

                /*
                printf("DEBUG actual %d expected %d\n",
                                (int)core_map_size(&table), inserted);
                                */

                TEST_UINT64_T_EQUALS(core_map_size(&table), inserted);

                value_bucket = core_map_get(&table, &key);

                TEST_POINTER_NOT_EQUALS(value_bucket, NULL);
            }
        }

        key = 9999;
        value_bucket = core_map_add(&table, &key);
        *value_bucket = 8888;

        core_map_iterator_init(&iterator, &table);

        i = 0;
        found = 0;

        while (core_map_iterator_has_next(&iterator)) {
            core_map_iterator_next(&iterator, (void **)&key_bucket,
                            (void **)&value_bucket);

            if (*key_bucket == 9999 && *value_bucket == 8888) {
                found = 1;
            }
            i++;
        }

        TEST_UINT64_T_EQUALS(i, core_map_size(&table));
        TEST_INT_EQUALS(found, 1);

        core_map_iterator_destroy(&iterator);

        core_map_destroy(&table);
    }

    {
        struct core_map table;
        int key;
        int *value;

        /*
        printf("-------------------\n");
        printf("DEBUG TEST-alpha-89\n");
        */

        core_map_init(&table, sizeof(int), sizeof(int));

        for (key = 0; key < 1000000; key++) {

/*
            printf("DEBUG key %d\n", key);
            */
            core_map_add(&table, &key);

            value = (int *)core_map_get(&table, &key);

            TEST_POINTER_NOT_EQUALS(value, NULL);

            *value = key;
        }

        core_map_destroy(&table);
    }

    {
        struct core_map map;
        int key;
        uint64_t elements;
        uint64_t i;
        void *buffer;
        int count;
        struct core_map map2;
        int *bucket;
        int value;

        elements = 900;
        core_map_init(&map, sizeof(int), sizeof(int));

        for (i = 0; i < elements; i++) {

            key = i;

            TEST_UINT64_T_EQUALS(core_map_size(&map), i);
            bucket = core_map_add(&map, &key);
            TEST_UINT64_T_EQUALS(core_map_size(&map), i + 1);

            (*bucket) = i * i;
        }

        count = core_map_pack_size(&map);
        buffer = core_memory_allocate(count, -1);
        core_map_pack(&map, buffer);

        /*
        printf("before unpacking %d bytes %p\n", count, buffer);
        */
        core_map_init(&map2, 0, 0);
        core_map_unpack(&map2, buffer);

        /*
        printf("after unpacking\n");
        */

        for (i = 0; i < elements; i++) {

            bucket = (int *)core_map_get(&map2, &i);

            TEST_POINTER_NOT_EQUALS(bucket, NULL);

            value = *bucket;

            TEST_INT_EQUALS(value, (int)(i * i));
        }

        /*
        printf("actual %d expected %d\n", (int)core_map_size(&map2), (int)core_map_size(&map));
        */

        TEST_UINT64_T_EQUALS(core_map_size(&map2), core_map_size(&map));
        TEST_UINT64_T_EQUALS(core_map_size(&map2), elements);

        core_memory_free(buffer, -1);

        core_map_destroy(&map2);
        core_map_destroy(&map);
    }

    {
        struct core_map map;
        struct core_map_iterator iterator;
        int i;
        int key;
        int value;
        int count;

        memset(&map, 1, sizeof(struct core_map));

        core_map_init(&map, sizeof(int), sizeof(int));

        count = 30000;

        for (i = 0; i < count; ++i) {
            core_map_add_value(&map, &i, &i);

            TEST_POINTER_NOT_EQUALS(core_map_get(&map, &i), NULL);

        }

        core_map_iterator_init(&iterator, &map);

        while (core_map_iterator_get_next_key_and_value(&iterator, &key, &value)) {

        }

        core_map_iterator_destroy(&iterator);

        core_map_destroy(&map);

    }
    END_TESTS();

    return 0;
}
