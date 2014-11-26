
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

    END_TESTS();

    return 0;
}
