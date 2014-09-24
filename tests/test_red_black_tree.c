
#include <core/structures/ordered/red_black_tree.h>

#include <core/system/memory_pool.h>

#include "test.h"

#include <stdlib.h>

int main(int argc, char **argv)
{
    BEGIN_TESTS();

    int i;
    int count;
    int key;
    struct bsal_red_black_tree tree;
    struct bsal_memory_pool memory_pool;

    bsal_memory_pool_init(&memory_pool, 1024*1024, -1);

    count = 100;

    srand(88);

    bsal_red_black_tree_init(&tree);
    bsal_red_black_tree_set_memory_pool(&tree, &memory_pool);

    for (i = 0; i < count; ++i) {
        key = rand();

        bsal_red_black_tree_add(&tree, key);
        TEST_INT_EQUALS(bsal_red_black_tree_has_ignored_rules(&tree), 0);
        TEST_INT_EQUALS(bsal_red_black_tree_size(&tree), i + 1);
    }

#if 0
    bsal_memory_pool_examine(&memory_pool);
#endif

    bsal_red_black_tree_destroy(&tree);

    TEST_INT_EQUALS(bsal_memory_pool_profile_balance_count(&memory_pool), 0);

    bsal_memory_pool_destroy(&memory_pool);

    END_TESTS();

    return 0;
}
