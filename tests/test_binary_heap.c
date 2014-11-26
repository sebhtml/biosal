
#include <core/structures/unordered/binary_heap.h>

#include <core/system/memory_pool.h>

#include "test.h"

#include <stdlib.h>

/*
*/
#define TEST_DELETE

int main(int argc, char **argv)
{
    struct core_binary_heap heap;
    int key;
    uint64_t value;
    int *key_pointer;
    uint64_t *value_pointer;
    int i;
    int minimum;
    int count;
    int last;

    BEGIN_TESTS();

    core_binary_heap_init(&heap, sizeof(int), 0,
                    CORE_BINARY_HEAP_MIN | CORE_BINARY_HEAP_INT_KEYS);

    TEST_INT_EQUALS(core_binary_heap_size(&heap), 0);

    count = 10000;
    /*
    count = 4;
    */

    i = 0;
    minimum = -1;

    while (i < count) {
        key = 9 + i;
        if (key < minimum || minimum == -1)
            minimum = key;

        value = 1234 * key;
        ++i;

        core_binary_heap_insert(&heap, &key, &value);

        TEST_INT_EQUALS(core_binary_heap_size(&heap), i);

        TEST_BOOLEAN_EQUALS(core_binary_heap_get_root(&heap, (void **)&key_pointer,
                                (void **)&value_pointer), TRUE);

        TEST_INT_EQUALS(*key_pointer, minimum);
        /*TEST_INT_EQUALS(*value_pointer, value);*/
    }

    last = -1;

    while (!core_binary_heap_empty(&heap)) {

        core_binary_heap_get_root(&heap, (void **)&key_pointer, NULL);
        key = *key_pointer;

        if (last == -1) {
            last = key;
        } else {
            /*
             * This test uses unique keys.
             */
            TEST_INT_NOT_EQUALS(last, key);
        }

        TEST_INT_IS_LOWER_THAN_OR_EQUAL(last, key);


        count = core_binary_heap_size(&heap);
        TEST_BOOLEAN_EQUALS(core_binary_heap_delete_root(&heap), TRUE);
        TEST_INT_EQUALS(core_binary_heap_size(&heap), count - 1);
    }

    core_binary_heap_destroy(&heap);

    {
        struct core_binary_heap heap2;
        int key;
        int value;
        int actual_sum;
        int expected_sum;
        int *key_pointer;
        int *value_pointer;
        int i;
        int count;

        core_binary_heap_init(&heap2, sizeof(key), sizeof(value), CORE_BINARY_HEAP_MAX);

        i = 0;
        count = 1000;
        expected_sum = 0;

        while (i < count) {
            key = count;
            value = i + 1;
            expected_sum += value;
            ++i;
            core_binary_heap_insert(&heap2, &key, &value);

            TEST_INT_EQUALS(core_binary_heap_size(&heap2), i);
        }

        key_pointer = NULL;
        value_pointer = NULL;

        actual_sum = 0;

        while (core_binary_heap_get_root(&heap2, (void **)&key_pointer,
                                (void **)&value_pointer)) {

            TEST_POINTER_NOT_EQUALS(key_pointer, NULL);
            TEST_POINTER_NOT_EQUALS(value_pointer, NULL);

            key = *key_pointer;
            value = *value_pointer;

            TEST_INT_EQUALS(key, count);

            TEST_INT_IS_GREATER_THAN_OR_EQUAL(value, 1);

            actual_sum += value;

            TEST_INT_IS_GREATER_THAN_OR_EQUAL(actual_sum, 1);
            TEST_INT_IS_LOWER_THAN_OR_EQUAL(actual_sum, expected_sum);

            key_pointer = NULL;
            value_pointer = NULL;

            TEST_BOOLEAN_EQUALS(core_binary_heap_delete_root(&heap2), TRUE);
        }

        TEST_INT_EQUALS(actual_sum, expected_sum);

        core_binary_heap_destroy(&heap2);
    }

    {
        struct core_binary_heap heap2;
        uint64_t key;
        size_t value;
        uint64_t *key_pointer;
        size_t *value_pointer;
        int i;
        int count;
        uint64_t previous;

        core_binary_heap_init(&heap2, sizeof(key), sizeof(value),
                        CORE_BINARY_HEAP_MIN | CORE_BINARY_HEAP_UINT64_T_KEYS);

        i = 0;
        count = 1000;

        while (i < count) {
            key = i * 2 + 1;
            value = i + 1;
            ++i;
            core_binary_heap_insert(&heap2, &key, &value);

            TEST_INT_EQUALS(core_binary_heap_size(&heap2), i);
        }

        key_pointer = NULL;
        value_pointer = NULL;

        previous = 0;

        while (core_binary_heap_get_root(&heap2, (void **)&key_pointer,
                                (void **)&value_pointer)) {

            TEST_POINTER_NOT_EQUALS(key_pointer, NULL);
            TEST_POINTER_NOT_EQUALS(value_pointer, NULL);

            key = *key_pointer;
            value = *value_pointer;

            if (previous == 0) {
                previous = key;
            } else {
                TEST_BOOLEAN_EQUALS(previous <= key, TRUE);
            }

            TEST_INT_IS_GREATER_THAN_OR_EQUAL(value, 1);

            key_pointer = NULL;
            value_pointer = NULL;

            TEST_BOOLEAN_EQUALS(core_binary_heap_delete_root(&heap2), TRUE);
        }

        core_binary_heap_destroy(&heap2);
    }

    END_TESTS();

    return 0;
}
