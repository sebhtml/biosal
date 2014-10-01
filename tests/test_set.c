
#include <core/structures/set.h>
#include <core/structures/set_iterator.h>

#include "test.h"

int main(int argc, char **argv)
{
    BEGIN_TESTS();

    struct core_set set;
    struct core_set_iterator iterator;
    int key;
    int i;
    int j;
    int found;
    int iterated;
    int *bucket;
    int sum;
    int sum2;
    int elements;

    elements = 900;

    sum = 0;
    core_set_init(&set, sizeof(key));

    key = 9;
    core_set_add(&set, &key);
    core_set_delete(&set, &key);
    core_set_delete(&set, &key);

    key = 0;

    for (i = 0; i < elements; i++) {

        key = i;
        sum += key;

        /*
        printf("insert %d\n", key);

        printf("GT actual %d expected %d\n", (int)core_set_size(&set), i);
        */
        TEST_INT_EQUALS(core_set_size(&set), i);
        core_set_add(&set, &key);

        TEST_INT_EQUALS(core_set_size(&set), i + 1);

        for (j = 0; j <= i; j++) {

            key = j;

            found = core_set_find(&set, &key);

            TEST_INT_IS_GREATER_THAN(found, 0);
        }

        /*
        printf("actual %d expected %d\n", (int)core_set_size(&set), i + 1);
        */

        TEST_INT_EQUALS(core_set_size(&set), i + 1);
    }

    iterated = 0;

    core_set_iterator_init(&iterator, &set);
    sum2 = 0;

    while (core_set_iterator_has_next(&iterator)) {

        core_set_iterator_next(&iterator, (void **)&bucket);
        key = *bucket;

        sum2 += key;

        iterated++;
    }

    TEST_INT_EQUALS(iterated, elements);
    core_set_iterator_destroy(&iterator);

    for (i = 0; i < elements; i++) {

        key = i;

        core_set_delete(&set, &key);

        found = core_set_find(&set, &key);

        TEST_INT_EQUALS(found, 0);

    }

    TEST_INT_EQUALS(core_set_size(&set), 0);

    END_TESTS();

    return 0;
}
