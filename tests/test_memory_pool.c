
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
#ifdef VERBOSE
    uint64_t elapsed;
#endif

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
#ifdef VERBOSE
    elapsed = core_timer_get_elapsed_nanoseconds(&timer);

    if (memory == NULL) {
        printf("Not using memory pool... ");
    } else {
        printf("Using memory pool... ");
    }
    printf("Elapsed : %" PRIu64 " milliseconds\n", elapsed / 1000 / 1000);
#endif

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

        core_memory_pool_init(&memory, 16777216, -1);

        test_allocator(&memory);

        core_memory_pool_disable_tracking(&memory);
        test_allocator(&memory);
        test_allocator(NULL);

        core_memory_pool_destroy(&memory);
    }

    struct core_memory_pool pool;
    int block_size;
    int name;
    int i;
    void *object;
    struct core_vector vector;

    name = 123;
    block_size = 16777216;

    core_memory_pool_init(&pool, block_size, name);

    core_vector_init(&vector, sizeof(void *));

    TEST_BOOLEAN_EQUALS(core_memory_pool_has_double_free(&pool), 0);
    TEST_BOOLEAN_EQUALS(core_memory_pool_has_leaks(&pool), 0);

    /*
     * With tracking
     */
    i = 9999;
    while (i--) {
        object = core_memory_pool_allocate(&pool, 77);

        TEST_POINTER_NOT_EQUALS(object, NULL);

        core_vector_push_back(&vector, &object);
    }

    for (i = 0; i < (int)core_vector_size(&vector); ++i) {
        object = core_vector_at_as_void_pointer(&vector, i);

        TEST_POINTER_NOT_EQUALS(object, NULL);

        core_memory_pool_free(&pool, object);

        TEST_BOOLEAN_EQUALS(core_memory_pool_has_double_free(&pool), 0);
    }

    TEST_BOOLEAN_EQUALS(core_memory_pool_has_double_free(&pool), 0);
    TEST_BOOLEAN_EQUALS(core_memory_pool_has_leaks(&pool), 0);

    /*
     * big segments
     */

    object = core_memory_pool_allocate(&pool, block_size * 10);

    TEST_POINTER_NOT_EQUALS(object, NULL);

    core_memory_pool_free(&pool, object);

    TEST_BOOLEAN_EQUALS(core_memory_pool_has_double_free(&pool), 0);
    TEST_BOOLEAN_EQUALS(core_memory_pool_has_leaks(&pool), 0);

    core_memory_pool_disable_tracking(&pool);

    core_vector_clear(&vector);

    /*
     * Without tracking
     */

    i = 9999;
    while (i--) {
        object = core_memory_pool_allocate(&pool, 77);

        TEST_POINTER_NOT_EQUALS(object, NULL);

        core_vector_push_back(&vector, &object);
    }

    for (i = 0; i < (int)core_vector_size(&vector); ++i) {
        object = core_vector_at_as_void_pointer(&vector, i);

        TEST_POINTER_NOT_EQUALS(object, NULL);

        core_memory_pool_free(&pool, object);

        TEST_BOOLEAN_EQUALS(core_memory_pool_has_double_free(&pool), 0);
    }

    TEST_BOOLEAN_EQUALS(core_memory_pool_has_double_free(&pool), 0);
    TEST_BOOLEAN_EQUALS(core_memory_pool_has_leaks(&pool), 0);

    /*
     * big segments
     */

    i = 4;

    while (i--) {
        object = core_memory_pool_allocate(&pool, block_size * 10);

        TEST_POINTER_NOT_EQUALS(object, NULL);

        core_memory_pool_free(&pool, object);
        TEST_BOOLEAN_EQUALS(core_memory_pool_has_leaks(&pool), 0);
    }

    TEST_BOOLEAN_EQUALS(core_memory_pool_has_double_free(&pool), 0);
    TEST_BOOLEAN_EQUALS(core_memory_pool_has_leaks(&pool), 0);

    core_memory_pool_destroy(&pool);
    core_vector_destroy(&vector);

    END_TESTS();

    return 0;
}
