
#include "test.h"

#include <core/structures/vector.h>

#include <core/system/timer.h>
#include <core/system/memory_pool.h>

#include <stdint.h>
#include <inttypes.h>

void test_allocator(struct core_memory_pool *memory)
{
    int i;
    int size;
    void *pointer;
    struct core_vector vector;
    struct core_timer timer;
    uint64_t elapsed;

    i = 1000000;
    size = 45;

    core_vector_init(&vector, sizeof(void *));
    core_timer_init(&timer);

    core_timer_start(&timer);

    while (i--) {
        if (memory != NULL) {
            pointer = core_memory_pool_allocate(memory, size);
        } else {
            pointer = core_memory_allocate(size, -1);
        }

        core_vector_push_back(&vector, &pointer);
    }

    core_timer_stop(&timer);
    elapsed = core_timer_get_elapsed_nanoseconds(&timer);

    if (memory == NULL) {
        printf("Not using memory pool... ");
    } else {
        printf("Using memory pool... ");
    }
    printf("Elapsed : %" PRIu64 " milliseconds\n", elapsed / 1000 / 1000);

    size = core_vector_size(&vector);
    for (i = 0; i < size; ++i) {
        pointer = core_vector_at_as_void_pointer(&vector, i);

        if (memory != NULL) {
            core_memory_pool_free(memory, pointer);
        } else {
            core_memory_free(pointer, -1);
        }
    }
    core_vector_destroy(&vector);
    core_timer_destroy(&timer);
}

int main(int argc, char **argv)
{
    BEGIN_TESTS();

    {
        struct core_memory_pool memory;

        core_memory_pool_init(&memory, 16777216, CORE_MEMORY_POOL_NAME_OTHER);

        test_allocator(&memory);

        core_memory_pool_disable_tracking(&memory);
        test_allocator(&memory);
        test_allocator(NULL);

        core_memory_pool_destroy(&memory);
    }

    END_TESTS();

    return 0;
}
