
#include <core/structures/string.h>

#include <string.h>

#include "test.h"

int main(int argc, char **argv)
{
    BEGIN_TESTS();

    struct core_string string;

    core_string_init(&string, NULL);

    TEST_POINTER_EQUALS(core_string_get(&string), NULL);

    TEST_INT_EQUALS(core_string_length(&string), 0);

    core_string_append(&string, "foo");

    TEST_INT_EQUALS(core_string_length(&string), 3);

    core_string_destroy(&string);

    {
        char sequence1[] = "0123456789";
        char expected1[] = "9876543210";
        char sequence2[] = "0123456789";
        char expected2[] = "3456789012";

        core_string_reverse_c_string(sequence1, 0, strlen(sequence1) - 1);
        TEST_INT_EQUALS(strcmp(sequence1, expected1), 0);

        core_string_rotate_c_string(sequence2, strlen(sequence2), 3);
        TEST_INT_EQUALS(strcmp(sequence2, expected2), 0);
    }
    END_TESTS();

    return 0;
}
