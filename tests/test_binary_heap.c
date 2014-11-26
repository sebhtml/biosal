
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

    BEGIN_TESTS();

    core_binary_heap_init(&heap, sizeof(int), sizeof(uint64_t),
                    CORE_BINARY_HEAP_MIN | CORE_BINARY_HEAP_INT_KEYS);

    TEST_INT_EQUALS(core_binary_heap_size(&heap), 0);

    core_binary_heap_destroy(&heap);

    END_TESTS();

    return 0;
}
