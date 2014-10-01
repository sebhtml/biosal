
#include <core/structures/set.h>
#include <core/structures/set_iterator.h>

#include "test.h"

int main(int argc, char **argv)
{
    BEGIN_TESTS();

    struct biosal_set set;
    struct biosal_set_iterator iterator;
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
    biosal_set_init(&set, sizeof(key));

    key = 9;
    biosal_set_add(&set, &key);
    biosal_set_delete(&set, &key);
    biosal_set_delete(&set, &key);

    key = 0;

    for (i = 0; i < elements; i++) {

        key = i;
        sum += key;

        /*
        printf("insert %d\n", key);

        printf("GT actual %d expected %d\n", (int)biosal_set_size(&set), i);
        */
        TEST_INT_EQUALS(biosal_set_size(&set), i);
        biosal_set_add(&set, &key);

        TEST_INT_EQUALS(biosal_set_size(&set), i + 1);

        for (j = 0; j <= i; j++) {

            key = j;

            found = biosal_set_find(&set, &key);

            TEST_INT_IS_GREATER_THAN(found, 0);
        }

        /*
        printf("actual %d expected %d\n", (int)biosal_set_size(&set), i + 1);
        */

        TEST_INT_EQUALS(biosal_set_size(&set), i + 1);
    }

    iterated = 0;

    biosal_set_iterator_init(&iterator, &set);
    sum2 = 0;

    while (biosal_set_iterator_has_next(&iterator)) {

        biosal_set_iterator_next(&iterator, (void **)&bucket);
        key = *bucket;

        sum2 += key;

        iterated++;
    }

    TEST_INT_EQUALS(iterated, elements);
    biosal_set_iterator_destroy(&iterator);

    for (i = 0; i < elements; i++) {

        key = i;

        biosal_set_delete(&set, &key);

        found = biosal_set_find(&set, &key);

        TEST_INT_EQUALS(found, 0);

    }

    TEST_INT_EQUALS(biosal_set_size(&set), 0);

    END_TESTS();

    return 0;
}
