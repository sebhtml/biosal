
#include "test.h"

#include <core/structures/free_list.h>
#include <core/system/memory.h>

#include <stdint.h>

int main(int argc, char **argv)
{
    BEGIN_TESTS();

    uint64_t *element;
    struct core_free_list list;
    int expected;
    int actual;
    int i;

    core_free_list_init(&list);

    TEST_BOOLEAN_EQUALS(core_free_list_empty(&list), 1);

    expected = 500;
    i = 0;

    while (i <  expected) {
        element = core_memory_allocate(sizeof(*element), -1);

        core_free_list_add(&list, element);

        ++i;

        TEST_INT_EQUALS(core_free_list_size(&list), i);
    }

    TEST_BOOLEAN_EQUALS(!core_free_list_empty(&list), 1);

    actual = 0;
    while (1) {
        element = core_free_list_remove(&list);

        if (element == NULL)
            break;

        ++actual;

        core_memory_free(element, -1);
        element = NULL;
    }

    TEST_BOOLEAN_EQUALS(core_free_list_empty(&list), 1);

    TEST_INT_EQUALS(actual, expected);

    core_free_list_destroy(&list);

    END_TESTS();

    return 0;
}
