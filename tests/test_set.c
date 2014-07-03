
#include <core/structures/set.h>
#include <core/structures/set_iterator.h>

#include "test.h"

int main(int argc, char **argv)
{
    BEGIN_TESTS();

    struct bsal_set set;
    struct bsal_set_iterator iterator;
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

    key = 0;
    sum = 0;
    bsal_set_init(&set, sizeof(key));

    for (i = 0; i < elements; i++) {

        key = i;
        sum += key;

        /*
        printf("insert %d\n", key);

        printf("GT actual %d expected %d\n", (int)bsal_set_size(&set), i);
        */
        TEST_INT_EQUALS(bsal_set_size(&set), i);
        bsal_set_add(&set, &key);

        TEST_INT_EQUALS(bsal_set_size(&set), i + 1);

        for (j = 0; j <= i; j++) {

            key = j;

            found = bsal_set_find(&set, &key);

            TEST_INT_IS_GREATER_THAN(found, 0);
        }

        /*
        printf("actual %d expected %d\n", (int)bsal_set_size(&set), i + 1);
        */

        TEST_INT_EQUALS(bsal_set_size(&set), i + 1);
    }

    iterated = 0;

    bsal_set_iterator_init(&iterator, &set);
    sum2 = 0;

    while (bsal_set_iterator_has_next(&iterator)) {

        bsal_set_iterator_next(&iterator, (void **)&bucket);
        key = *bucket;

        sum2 += key;

        iterated++;
    }

    TEST_INT_EQUALS(iterated, elements);
    bsal_set_iterator_destroy(&iterator);

    for (i = 0; i < elements; i++) {

        key = i;

        bsal_set_delete(&set, &key);

        found = bsal_set_find(&set, &key);

        TEST_INT_EQUALS(found, 0);

    }

    TEST_INT_EQUALS(bsal_set_size(&set), 0);

    END_TESTS();

    return 0;
}
