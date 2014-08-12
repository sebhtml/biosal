
#include <core/structures/string.h>

#include "test.h"

int main(int argc, char **argv)
{
    BEGIN_TESTS();

    struct bsal_string string;

    bsal_string_init(&string, NULL);

    TEST_POINTER_EQUALS(bsal_string_get(&string), NULL);

    TEST_INT_EQUALS(bsal_string_length(&string), 0);

    bsal_string_append(&string, "foo");

    TEST_INT_EQUALS(bsal_string_length(&string), 3);

    bsal_string_destroy(&string);

    END_TESTS();

    return 0;
}
