
#include <core/structures/ordered/red_black_tree.h>

#include <core/structures/vector.h>

#include <core/system/memory_pool.h>

#include "test.h"

#include <stdlib.h>

/*
*/
#define TEST_DELETE

int main(int argc, char **argv)
{
    BEGIN_TESTS();

    int i;
    int lowest;
    int count;
    int key;
    struct biosal_red_black_tree tree;
    struct biosal_memory_pool memory_pool;
    struct biosal_vector keys;
    int *result;
    int size;

    lowest = 0;
    biosal_memory_pool_init(&memory_pool, 1024*1024, -1);

    biosal_vector_init(&keys, sizeof(int));

#if 0
#endif
    count = 100000;
#if 0
    count = 1;
#endif

    srand(88);

    biosal_red_black_tree_init(&tree, sizeof(int), sizeof(int), &memory_pool);

#if 0
    biosal_red_black_tree_set_memory_pool(&tree, &memory_pool);
#endif

#if 0
    biosal_red_black_tree_print(&tree);
#endif

    for (i = 0; i < count; ++i) {
        key = rand() % (2 * count);

        biosal_vector_push_back(&keys, &key);

        if (i == 0)
            lowest = key;

        if (key < lowest)
            lowest = key;

#if 0
        printf("add node %d (tree before)\n", key);
        biosal_red_black_tree_print(&tree);
#endif
        TEST_INT_EQUALS(biosal_red_black_tree_has_ignored_rules(&tree), 0);
        biosal_red_black_tree_add(&tree, &key);
        TEST_INT_EQUALS(biosal_red_black_tree_has_ignored_rules(&tree), 0);
        TEST_INT_EQUALS(biosal_red_black_tree_size(&tree), i + 1);

        TEST_POINTER_NOT_EQUALS(biosal_red_black_tree_get(&tree, &key), NULL);

        result = biosal_red_black_tree_get_lowest_key(&tree);
        TEST_POINTER_NOT_EQUALS(result, NULL);

#if 0
        biosal_red_black_tree_run_assertions(&tree);
#endif

#if 0
        biosal_red_black_tree_print(&tree);
        TEST_INT_EQUALS(*result, lowest);
#endif

#if 0
        biosal_red_black_tree_print(&tree);
#endif
    }

#if 0
    biosal_red_black_tree_print(&tree);
#endif

#if 0
    biosal_memory_pool_examine(&memory_pool);
#endif

    biosal_red_black_tree_run_assertions(&tree);

    count = 0;
    size = count;

    for (i = 0; i < count; ++i) {
        key = biosal_vector_at_as_int(&keys, i);

        result = biosal_red_black_tree_get(&tree, &key);

        TEST_POINTER_NOT_EQUALS(result, NULL);

#ifdef TEST_DELETE
        biosal_red_black_tree_delete(&tree, &key);
        --size;
#endif

        TEST_INT_EQUALS(biosal_red_black_tree_size(&tree), size);
    }

    biosal_red_black_tree_destroy(&tree);
    biosal_vector_destroy(&keys);

    TEST_INT_EQUALS(biosal_memory_pool_profile_balance_count(&memory_pool), 0);

    biosal_memory_pool_destroy(&memory_pool);

    /*
     * Small deletions
     */
    {

        struct biosal_red_black_tree tree;
        struct biosal_memory_pool memory_pool;
        struct biosal_vector keys;

        biosal_vector_init(&keys, sizeof(int));

        biosal_memory_pool_init(&memory_pool, 1024*1024, -1);
        biosal_red_black_tree_init(&tree, sizeof(int), sizeof(int), &memory_pool);

        i = 0;
        size = 100000;

        while (i < size) {
            key = rand() % (2 * size);

            /*
             * For this test, we don't want duplicates.
             */
            while (biosal_red_black_tree_get(&tree, &key) != NULL) {
                key = rand() % (2 * size);
            }
#if 0
            printf("Add %d\n", key);
#endif
            biosal_red_black_tree_add(&tree, &key);
#if 0
            biosal_red_black_tree_print(&tree);
#endif

            TEST_POINTER_NOT_EQUALS(biosal_red_black_tree_get(&tree, &key), NULL);

            TEST_POINTER_NOT_EQUALS(biosal_red_black_tree_get_lowest_key(&tree), NULL);
            result = biosal_red_black_tree_get(&tree, &key);
            *result = i;
            biosal_vector_push_back(&keys, &key);

            ++i;
        }

        i = 0;

#if 0
        biosal_red_black_tree_print(&tree);
#endif
        while (i < size) {
            key = biosal_vector_at_as_int(&keys, i);
/*
            printf("Delete %d\n", key);
            */

            TEST_POINTER_NOT_EQUALS(biosal_red_black_tree_get(&tree, &key), NULL);

            biosal_red_black_tree_delete(&tree, &key);
            TEST_POINTER_EQUALS(biosal_red_black_tree_get(&tree, &key), NULL);
            ++i;
        }
        biosal_red_black_tree_destroy(&tree);
        biosal_vector_destroy(&keys);

        TEST_INT_EQUALS(biosal_memory_pool_profile_balance_count(&memory_pool), 0);

        biosal_memory_pool_destroy(&memory_pool);
    }

    END_TESTS();

    return 0;
}
