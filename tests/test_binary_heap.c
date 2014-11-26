
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

    BEGIN_TESTS();

    core_binary_heap_init(&heap, sizeof(int), 0,
                    CORE_BINARY_HEAP_MIN | CORE_BINARY_HEAP_INT_KEYS);

    TEST_INT_EQUALS(core_binary_heap_size(&heap), 0);

    count = 10000;
    /*
    count = 4;
    */

    i = 0;
    minimum = 999999999;

    while (i < count) {
        key = 9 + i;
        if (key < minimum)
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

    core_binary_heap_destroy(&heap);

    END_TESTS();

    return 0;
}
