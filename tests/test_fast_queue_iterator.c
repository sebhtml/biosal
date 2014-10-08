
#include <core/structures/fast_queue.h>
#include <core/structures/fast_queue_iterator.h>

#include <core/structures/vector.h>

#include "test.h"

int main(int argc, char **argv)
{
    BEGIN_TESTS();

    struct core_fast_queue queue;
    struct core_fast_queue_iterator iterator;
    int sum;
    int value;
    int size;
    int i;
    int value2;
    int sum2;
    struct core_vector vector;

    srand(42);

    core_vector_init(&vector, sizeof(int));
    core_fast_queue_init(&queue, sizeof(int));

    size = 150;

    /*
     * We don't care about overflows here.
     */
    sum = 0;

    for (i = 0; i < size; ++i) {

        value = rand();
        core_fast_queue_enqueue(&queue, &value);
        sum += value;

        core_vector_push_back_int(&vector, value);
    }

    TEST_INT_EQUALS(core_fast_queue_size(&queue), size);

    sum2 = 0;

    core_fast_queue_iterator_init(&iterator, &queue);

    while (core_fast_queue_iterator_next_value(&iterator, &value)) {

        sum2 += value;
    }

    core_fast_queue_iterator_destroy(&iterator);

    TEST_INT_EQUALS(sum2, sum);

    /*
     * Check order
     */
    core_fast_queue_iterator_init(&iterator, &queue);

    i = 0;

    while (core_fast_queue_iterator_next_value(&iterator, &value)) {

        value2 = core_vector_at_as_int(&vector, i);
        ++i;

        TEST_INT_EQUALS(value, value2);
    }

    core_fast_queue_iterator_destroy(&iterator);

    core_fast_queue_destroy(&queue);
    core_vector_destroy(&vector);

    END_TESTS();

    return 0;
}
