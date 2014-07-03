
#include "test.h"

#include <core/engine/node.h>

#include <string.h>

int main(int argc, char **argv)
{
    BEGIN_TESTS();

    struct bsal_node node;
    char buffer[1024];

    strcpy(buffer, "0,1");
    TEST_INT_EQUALS(bsal_node_threads_from_string(&node, buffer, 0), 0);
    TEST_INT_EQUALS(bsal_node_threads_from_string(&node, buffer, 1), 1);

    strcpy(buffer, "2,3,4,5");
    TEST_INT_EQUALS(bsal_node_threads_from_string(&node, buffer, 2), 4);
    TEST_INT_EQUALS(bsal_node_threads_from_string(&node, buffer, 4), 2);
    TEST_INT_EQUALS(bsal_node_threads_from_string(&node, buffer, 5), 3);
    TEST_INT_EQUALS(bsal_node_threads_from_string(&node, buffer, 3), 5);

    /* Knights Corner with host Xeon */
    strcpy(buffer, "32,244");
    TEST_INT_EQUALS(bsal_node_threads_from_string(&node, buffer, 0), 32);
    TEST_INT_EQUALS(bsal_node_threads_from_string(&node, buffer, 1), 244);
    TEST_INT_EQUALS(bsal_node_threads_from_string(&node, buffer, 120), 32);
    TEST_INT_EQUALS(bsal_node_threads_from_string(&node, buffer, 121), 244);
    TEST_INT_EQUALS(bsal_node_threads_from_string(&node, buffer, 1022), 32);
    TEST_INT_EQUALS(bsal_node_threads_from_string(&node, buffer, 1023), 244);
    TEST_INT_EQUALS(bsal_node_threads_from_string(&node, buffer, 1001022), 32);
    TEST_INT_EQUALS(bsal_node_threads_from_string(&node, buffer, 1001023), 244);

    END_TESTS();

    return 0;
}
