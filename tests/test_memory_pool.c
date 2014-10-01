
#include "test.h"

#include <core/structures/vector.h>

#include <core/system/timer.h>
#include <core/system/memory_pool.h>

#include <stdint.h>
#include <inttypes.h>

void test_allocator(struct biosal_memory_pool *memory)
{
    int i;
    int size;
    void *pointer;
    struct biosal_vector vector;
    struct biosal_timer timer;
    uint64_t elapsed;

    i = 1000000;
    size = 45;

    biosal_vector_init(&vector, sizeof(void *));
    biosal_timer_init(&timer);

    biosal_timer_start(&timer);

    while (i--) {
        if (memory != NULL) {
            pointer = biosal_memory_pool_allocate(memory, size);
        } else {
            pointer = biosal_memory_allocate(size, -1);
        }

        biosal_vector_push_back(&vector, &pointer);
    }

    biosal_timer_stop(&timer);
    elapsed = biosal_timer_get_elapsed_nanoseconds(&timer);

    if (memory == NULL) {
        printf("Not using memory pool... ");
    } else {
        printf("Using memory pool... ");
    }
    printf("Elapsed : %" PRIu64 " milliseconds\n", elapsed / 1000 / 1000);

    size = biosal_vector_size(&vector);
    for (i = 0; i < size; ++i) {
        pointer = biosal_vector_at_as_void_pointer(&vector, i);

        if (memory != NULL) {
            biosal_memory_pool_free(memory, pointer);
        } else {
            biosal_memory_free(pointer, -1);
        }
    }
    biosal_vector_destroy(&vector);
    biosal_timer_destroy(&timer);
}

int main(int argc, char **argv)
{
    BEGIN_TESTS();

    {
        struct biosal_memory_pool memory;

        biosal_memory_pool_init(&memory, 16777216, BIOSAL_MEMORY_POOL_NAME_OTHER);

        test_allocator(&memory);

        biosal_memory_pool_disable_tracking(&memory);
        test_allocator(&memory);
        test_allocator(NULL);

        biosal_memory_pool_destroy(&memory);
    }

    END_TESTS();

    return 0;
}
