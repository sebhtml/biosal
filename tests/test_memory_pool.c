
#include "test.h"

#include <core/structures/vector.h>

#include <core/system/timer.h>
#include <core/system/memory_pool.h>

#include <stdint.h>
#include <inttypes.h>

void test_allocator(struct bsal_memory_pool *memory)
{
    int i;
    int size;
    void *pointer;
    struct bsal_vector vector;
    struct bsal_timer timer;
    uint64_t elapsed;

    i = 1000000;
    size = 45;

    bsal_vector_init(&vector, sizeof(void *));
    bsal_timer_init(&timer);

    bsal_timer_start(&timer);

    while (i--) {
        if (memory != NULL) {
            pointer = bsal_memory_pool_allocate(memory, size);
        } else {
            pointer = bsal_memory_allocate(size);
        }

        bsal_vector_push_back(&vector, &pointer);
    }

    bsal_timer_stop(&timer);
    elapsed = bsal_timer_get_elapsed_nanoseconds(&timer);

    if (memory == NULL) {
        printf("Not using memory pool... ");
    } else {
        printf("Using memory pool... ");
    }
    printf("Elapsed : %" PRIu64 " milliseconds\n", elapsed / 1000 / 1000);

    bsal_vector_destroy(&vector);
    bsal_timer_destroy(&timer);
}

int main(int argc, char **argv)
{
    BEGIN_TESTS();

    {
        struct bsal_memory_pool memory;

        bsal_memory_pool_init(&memory, 16777216);

        test_allocator(&memory);

        bsal_memory_pool_disable_tracking(&memory);
        test_allocator(&memory);
        test_allocator(NULL);

        bsal_memory_pool_destroy(&memory);
    }

    END_TESTS();

    return 0;
}
