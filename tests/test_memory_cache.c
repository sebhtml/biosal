
#include "test.h"

#include <core/system/memory_cache.h>

#include <core/structures/vector.h>

#include <stdint.h>
#include <inttypes.h>

int main(int argc, char **argv)
{
    BEGIN_TESTS();

    struct core_memory_cache cache;
    size_t size = 5;
    size_t chunk_size = 8388608;
    void *pointer;
    void *old_pointer;
    struct core_vector vector;
    int i;

    core_memory_cache_init(&cache, -1, size, chunk_size);

    TEST_INT_EQUALS(core_memory_cache_balance(&cache), 0);

    pointer = core_memory_cache_allocate(&cache, size);
    TEST_POINTER_NOT_EQUALS(pointer, NULL);
    old_pointer = pointer;

    TEST_INT_EQUALS(core_memory_cache_balance(&cache), 1);

    core_memory_cache_free(&cache, pointer);
    pointer = core_memory_cache_allocate(&cache, size);

    TEST_POINTER_EQUALS(pointer, old_pointer);

    core_memory_cache_free(&cache, pointer);
    TEST_INT_EQUALS(core_memory_cache_balance(&cache), 0);

    core_vector_init(&vector, sizeof(void *));

    for (i = 0; i < 2000000; ++i) {

        pointer = core_memory_cache_allocate(&cache, size);
        TEST_POINTER_NOT_EQUALS(pointer, NULL);

        core_vector_push_back(&vector, &pointer);
    }
    core_memory_cache_destroy(&cache);

    core_vector_destroy(&vector);
    END_TESTS();

    return 0;
}
