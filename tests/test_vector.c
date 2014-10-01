
#include <core/structures/vector.h>
#include <core/structures/vector_iterator.h>

#include <core/helpers/vector_helper.h>
#include <core/system/memory.h>

#include "test.h"

int main(int argc, char **argv)
{
    /*biosal_memory_allocate(0, -1);*/
    /*biosal_memory_allocate(-1, -1);*/
    /*biosal_memory_allocate(-10000, -1);*/

    BEGIN_TESTS();

    {
        struct biosal_vector vector;
        int i;
        int value;
        int *pointer;
        int expected;
        int actual;
        int j;

#if 0
        struct biosal_vector vector2;

        biosal_vector_init(&vector2, sizeof(int));
        j = 9;
        printf("Pushback, before\n");
        biosal_vector_push_back(&vector2, &j);
        printf("Pushback, after\n");
        biosal_vector_destroy(&vector2);
        return 0;
#endif


        biosal_vector_init(&vector, sizeof(int));
        TEST_INT_EQUALS(biosal_vector_size(&vector), 0);

        for (i = 0; i < 1000; i++) {
            TEST_POINTER_EQUALS(biosal_vector_at(&vector, i), NULL);

            /* Simulate an issue
            TEST_POINTER_EQUALS(biosal_vector_at(&vector, i), (void *)0x1);
             */

            biosal_vector_push_back(&vector, &i);
            TEST_INT_EQUALS(biosal_vector_size(&vector), i + 1);

            value = *(int *)biosal_vector_at(&vector, i);

            TEST_POINTER_NOT_EQUALS(biosal_vector_at(&vector, i), NULL);
            TEST_INT_EQUALS(value, i);

            pointer = (int *)biosal_vector_at(&vector, i);
            expected = i * i * i;
            *pointer = expected;

            value = *(int *)biosal_vector_at(&vector, i);

            /* restore the value */
            *pointer = i;

            TEST_INT_EQUALS(value, expected);

            for (j = 0; j <= i; j++) {
                expected = j;
                pointer = biosal_vector_at(&vector, j);
                actual = *pointer;
/*
                printf("DEBUG index %d actual %d expected %d bucket %p\n",
                                j, actual, expected, (void *)pointer);
                                */

                TEST_INT_EQUALS(actual, expected);
            }
        }

        biosal_vector_resize(&vector, 42);
        TEST_INT_EQUALS(biosal_vector_size(&vector), 42);

        TEST_POINTER_EQUALS(biosal_vector_at(&vector, 42), NULL);
        TEST_POINTER_EQUALS(biosal_vector_at(&vector, 43), NULL);
        TEST_POINTER_NOT_EQUALS(biosal_vector_at(&vector, 41), NULL);
        TEST_POINTER_NOT_EQUALS(biosal_vector_at(&vector, 0), NULL);
        TEST_POINTER_NOT_EQUALS(biosal_vector_at(&vector, 20), NULL);

        biosal_vector_resize(&vector, 100000);
        TEST_INT_EQUALS(biosal_vector_size(&vector), 100000);
        TEST_POINTER_NOT_EQUALS(biosal_vector_at(&vector, 100000 - 1), NULL);
        TEST_POINTER_NOT_EQUALS(biosal_vector_at(&vector, 0), NULL);
        TEST_POINTER_EQUALS(biosal_vector_at(&vector, 2000000), NULL);

        for (i = 0; i < 100000; i++) {

            expected = i * i;

            pointer = (int *)biosal_vector_at(&vector, i);

            TEST_POINTER_NOT_EQUALS(pointer, NULL);

            *pointer = expected;

            pointer = (int *)biosal_vector_at(&vector, i);
            actual = *pointer;

            TEST_INT_EQUALS(actual, expected);
        }

        biosal_vector_destroy(&vector);
    }

    {
        struct biosal_vector vector1;
        struct biosal_vector vector2;
        struct biosal_vector_iterator iterator;
        int i;
        void *buffer;
        int value1;
        int value2;
        int count;
        int *iterator_value;

        biosal_vector_init(&vector1, sizeof(int));

        for (i = 0; i < 99; i++) {
            biosal_vector_push_back(&vector1, &i);
        }

        biosal_vector_iterator_init(&iterator, &vector1);

        i = 0;

        while (biosal_vector_iterator_has_next(&iterator)) {
            biosal_vector_iterator_next(&iterator, (void **)&iterator_value);

            TEST_INT_EQUALS(*iterator_value, i);
            i++;
        }

        biosal_vector_iterator_destroy(&iterator);

        count = biosal_vector_pack_size(&vector1);

        TEST_INT_IS_GREATER_THAN(count, 0);

        buffer = biosal_memory_allocate(count, -1);

        biosal_vector_pack(&vector1, buffer);

        biosal_vector_init(&vector2, 0);
        biosal_vector_unpack(&vector2, buffer);

        for (i = 0; i < biosal_vector_size(&vector1); i++) {
            value1 = *(int *)biosal_vector_at(&vector1, i);
            value2 = *(int *)biosal_vector_at(&vector2, i);

            TEST_INT_EQUALS(value1, value2);
        }

        biosal_memory_free(buffer, - 1);
        biosal_vector_destroy(&vector1);
        biosal_vector_destroy(&vector2);
    }

    {
        int i;
        struct biosal_vector vector;
        int value1;
        int value2;

        biosal_vector_init(&vector, sizeof(int));

        i = 1000;

        while (i--) {
            biosal_vector_push_back(&vector, &i);

            if (biosal_vector_size(&vector) > 10 && 0) {
                break;
            }
        }

        /*
        biosal_vector_print_int(&vector);
        printf("\n");
        */

        biosal_vector_sort_int(&vector);

        /*
        biosal_vector_print_int(&vector);
        printf("\n");
        */

        for (i = 0; i < biosal_vector_size(&vector) - 1; i++) {
            value1 = biosal_vector_at_as_int(&vector, i);
            value2 = biosal_vector_at_as_int(&vector, i + 1);

            if (value2 < value1) {
                printf("%d %d\n", value1, value2);
            }

            TEST_BOOLEAN_EQUALS(value1 <= value2, 1);
        }

        biosal_vector_destroy(&vector);
    }

    END_TESTS();

    return 0;
}
