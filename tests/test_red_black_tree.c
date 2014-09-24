
#include <core/structures/ordered/red_black_tree.h>

#include "test.h"

#include <stdlib.h>

int main(int argc, char **argv)
{
    BEGIN_TESTS();

    int i;
    int count;
    int key;
    struct bsal_red_black_tree tree;

    count = 100;

    srand(88);

    bsal_red_black_tree_init(&tree);

    for (i = 0; i < count; ++i) {
        key = rand();

        bsal_red_black_tree_add(&tree, key);

        TEST_INT_EQUALS(bsal_red_black_tree_size(&tree), i + 1);
    }

    END_TESTS();

    bsal_red_black_tree_destroy(&tree);

    return 0;
}
