
#include "red_black_tree.h"
#include "red_black_node.h"

#include <core/system/memory_pool.h>
#include <core/system/debugger.h>

#include <stdlib.h>
#include <string.h>

/*
#define RUN_TREE_ASSERTIONS
*/

void core_red_black_tree_init(struct core_red_black_tree *self, int key_size, int value_size,
                struct core_memory_pool *memory_pool)
{
    self->memory_pool = memory_pool;
    self->size = 0;

    self->key_size = key_size;
    self->value_size = value_size;

    self->compare = core_red_black_tree_compare_memory_content;

#ifdef CORE_RED_BLACK_TREE_USE_CACHE_LAST
    self->cached_last_node = NULL;
#endif
#ifdef CORE_RED_BLACK_TREE_USE_CACHE_LOWEST
    self->cached_lowest_node = NULL;
#endif

#ifdef CORE_RED_BLACK_TREE_USE_NIL_NODE_LIST
    self->nil_node_list = NULL;
#endif

#ifdef CORE_RED_BLACK_TREE_USE_NORMAL_NODE_LIST
    self->normal_node_list = NULL;
#endif

    self->root = core_red_black_tree_allocate_nil_node(self);
}

void core_red_black_tree_destroy(struct core_red_black_tree *self)
{
#ifdef CORE_RED_BLACK_TREE_USE_NIL_NODE_LIST
    struct core_red_black_node *node;
#endif

    if (self->root != NULL) {
        core_red_black_tree_free_node(self, self->root);
        self->root = NULL;
    }

#ifdef CORE_RED_BLACK_TREE_USE_NIL_NODE_LIST
    while (self->nil_node_list != NULL) {
        node = self->nil_node_list->left_node;
        core_memory_pool_free(self->memory_pool, self->nil_node_list);
        self->nil_node_list = node;
    }
#endif

#ifdef CORE_RED_BLACK_TREE_USE_NORMAL_NODE_LIST
    while (self->normal_node_list != NULL) {
        node = self->normal_node_list->left_node;
        core_memory_pool_free(self->memory_pool, self->normal_node_list->key);
        core_memory_pool_free(self->memory_pool, self->normal_node_list->value);
        core_memory_pool_free(self->memory_pool, self->normal_node_list);
        self->normal_node_list = node;
    }
#endif

    self->memory_pool = NULL;
    self->size = 0;
    self->key_size = 0;
    self->value_size = 0;
}

void *core_red_black_tree_add_key_and_value(struct core_red_black_tree *self, void *key, void *value)
{
    void *new_value;

    new_value = core_red_black_tree_add(self, key);

    CORE_DEBUGGER_ASSERT(new_value != NULL);

    if (value != NULL)
        core_memory_copy(new_value, value, self->value_size);

    return new_value;
}

void *core_red_black_tree_add(struct core_red_black_tree *self, void *key)
{
    struct core_red_black_node *node;
    struct core_red_black_node *left_nil;
    struct core_red_black_node *right_nil;
    struct core_red_black_node *current_node;
    struct core_red_black_node *left_node;
    struct core_red_black_node *right_node;
    int result;
    int inserted;

    CORE_DEBUGGER_ASSERT(self->root != NULL);

    node = core_red_black_tree_allocate_normal_node(self, key, NULL);

    CORE_DEBUGGER_ASSERT_NOT_NULL(node->value);

#ifdef CORE_RED_BLACK_TREE_USE_CACHE_LAST
    self->cached_last_node = node;
#endif

    if (core_red_black_node_is_leaf(self->root)) {

        left_nil = self->root;
        right_nil = core_red_black_tree_allocate_nil_node(self);

        self->root = node;

        core_red_black_node_set_left_node(node, left_nil);
        core_red_black_node_set_right_node(node, right_nil);

        core_red_black_tree_insert_case1(self, node);

#ifdef CORE_RED_BLACK_TREE_USE_CACHE_LOWEST
        /*
         * This is the first item.
         * So this is the lowest key.
         */
        self->cached_lowest_node = node;
#endif

        ++self->size;

        CORE_DEBUGGER_ASSERT_NOT_NULL(node->value);

        return node->value;
    }

    current_node = self->root;
    inserted = 0;

    while (inserted == 0) {

        result = core_red_black_tree_compare(self, node->key, current_node->key);

        if (result < 0) {

            left_node = core_red_black_node_left_node(current_node);

            if (core_red_black_node_is_leaf(left_node)) {

                left_nil = left_node;
                right_nil = core_red_black_tree_allocate_nil_node(self);

                core_red_black_node_set_left_node(current_node, node);
                core_red_black_node_set_left_node(node, left_nil);
                core_red_black_node_set_right_node(node, right_nil);

                ++self->size;
                core_red_black_tree_insert_case1(self, node);
                inserted = 1;
            } else {
                current_node = left_node;
            }
        } else /* if (core_red_black_node_key(node) < core_red_black_node_key(current_node)) */ {

            right_node = core_red_black_node_right_node(current_node);

            if (core_red_black_node_is_leaf(right_node)) {

                left_nil = core_red_black_tree_allocate_nil_node(self);
                right_nil = right_node;

                core_red_black_node_set_right_node(current_node, node);
                core_red_black_node_set_left_node(node, left_nil);
                core_red_black_node_set_right_node(node, right_nil);

                ++self->size;
                core_red_black_tree_insert_case1(self, node);
                inserted = 1;
            } else {
                current_node = right_node;
            }
        }
    }

#ifdef RUN_TREE_ASSERTIONS
    /*
     * If the current node is RED, then the parent must be black
     */
    if (core_red_black_node_is_red(node)) {
        CORE_DEBUGGER_ASSERT(core_red_black_node_is_black(node->parent));
    }

    core_red_black_node_run_assertions(node, self);
#endif

#ifdef CORE_RED_BLACK_TREE_USE_CACHE_LOWEST
    /*
     * Maintain the cache for the lowest key.
     * To do so, we just need to check if the inserted key is lower than
     * the key for cached_lowest_node.
     *
     * This is O(1).
     */

    if (self->cached_lowest_node == NULL
              || core_red_black_tree_compare(self, node->key, self->cached_lowest_node->key) < 0) {

        self->cached_lowest_node = node;

        CORE_DEBUGGER_ASSERT(self->cached_lowest_node->key != NULL);
    }
#endif

    CORE_DEBUGGER_ASSERT_NOT_NULL(node->value);

    return node->value;
}

void core_red_black_tree_delete(struct core_red_black_tree *self, void *key)
{
    struct core_red_black_node *node;

    /*
     * Nothing to delete.
     */
    if (self->size == 0) {
        return;
    }

    /* Find the node
     */
    node = core_red_black_tree_get_node(self, key);

    /*
     * The node is not in the tree
     */
    if (node == NULL)
        return;

#ifdef CORE_RED_BLACK_TREE_USE_CACHE_LOWEST
    /*
     * Update the lowest value cache.
     */

    CORE_DEBUGGER_ASSERT(self->cached_lowest_node != NULL || self->cached_lowest_node->key != NULL);

    /*
     * If we are deleting the lowest key, the new lowest key
     * is the parent node for this key (which might be NULL).
     */
    if (self->cached_lowest_node != NULL
                    && core_red_black_tree_compare(self, key, self->cached_lowest_node->key) == 0) {
        self->cached_lowest_node = self->cached_lowest_node->parent;
    }

#endif

#ifdef CORE_RED_BLACK_TREE_USE_CACHE_LAST
    /*
     * Remove the cached element pointer.
     */
    if (self->cached_last_node != NULL
                    && core_red_black_tree_compare(self, key, self->cached_last_node->key) == 0) {

        self->cached_last_node = NULL;
    }
#endif

    core_red_black_tree_delete_private(self, node);
}

void core_red_black_tree_delete_private(struct core_red_black_tree *self,
                struct core_red_black_node *node)
{
    struct core_red_black_node *largest_value_node;

    /*
     * Find the largest value before the node
     */
    largest_value_node = node->left_node;

    if (!core_red_black_node_is_leaf(largest_value_node)) {

        while (!core_red_black_node_is_leaf(largest_value_node->right_node)) {
            largest_value_node = largest_value_node->right_node;
        }
    }

#if 0
    printf("Before Delete node %d\n",
                    core_red_black_node_get_key_as_int(node, self->key_size));

    core_red_black_tree_print(self);
#endif

    /*
     * Delete the node.
     */
    if (core_red_black_node_is_leaf(largest_value_node)) {

#if 0
        printf("delete node %d, no largest value found.\n",
                        core_red_black_node_get_key_as_int(node, self->key_size));
#endif
        core_red_black_tree_delete_one_child(self, node);

    } else {
        /*
         * Replace the key and the value of the node
         */

#if 0
        printf("Largest value node %d\n",
                    core_red_black_node_get_key_as_int(largest_value_node, self->key_size));
#endif

        core_memory_copy(node->key, largest_value_node->key, self->key_size);
        core_memory_copy(node->value, largest_value_node->value, self->value_size);

#if 0
        core_red_black_tree_print(self);
#endif
        core_red_black_tree_delete_one_child(self, largest_value_node);
    }

    --self->size;

#if 0
    printf("After Delete\n");
    core_red_black_tree_print(self);
#endif
}

/*
 * This function checks the rules.
 *
 * \see http://en.wikipedia.org/wiki/Red%E2%80%93black_tree
 *
 * The 5 rules are:
 *
 * 1. A node is red or black.
 * 2. The root is black.
 * 3. All leaf nodes are black.
 * 4. Any red node has 2 black child nodes.
 * 5. Every path from given node to any of its descendant leaf node contains
 *    the same number of black nodes.
 */
int core_red_black_tree_has_ignored_rules(struct core_red_black_tree *self)
{
    if (self->root == NULL) {
        return 0;
    }

    if (core_red_black_node_color(self->root) != CORE_COLOR_BLACK) {
        printf("Rule 2 is ignored !\n");
        return 1;
    }

    return 0;
}

void core_red_black_tree_set_memory_pool(struct core_red_black_tree *self,
                struct core_memory_pool *memory_pool)
{
    self->memory_pool = memory_pool;
}

int core_red_black_tree_size(struct core_red_black_tree *self)
{
    return self->size;
}

void core_red_black_tree_free_node(struct core_red_black_tree *self,
                struct core_red_black_node *node)
{
    struct core_red_black_node *left_node;
    struct core_red_black_node *right_node;

    if (node == NULL) {
        return;
    }

#if 0
    printf("free_node\n");
#endif

    left_node = core_red_black_node_left_node(node);
    right_node = core_red_black_node_right_node(node);

    core_red_black_tree_free_node(self, left_node);
    core_red_black_tree_free_node(self, right_node);

    if (node->key != NULL) {
        core_memory_pool_free(self->memory_pool, node->key);
        core_memory_pool_free(self->memory_pool, node->value);
    }
    core_red_black_node_destroy(node);

#if 0
    printf("free %p\n", (void *)node);
#endif
    core_memory_pool_free(self->memory_pool, node);
}

/*
 * Case 1: the node is the root.
 *
 * \see http://en.wikipedia.org/wiki/Red%E2%80%93black_tree
 */
void core_red_black_tree_insert_case1(struct core_red_black_tree *self,
                struct core_red_black_node *node)
{
    if (node->parent == NULL) {
        /*
         * Adding at the root adds one black node to all paths.
         */
        core_red_black_node_set_color(self->root, CORE_COLOR_BLACK);
    } else {
        core_red_black_tree_insert_case2(self, node);
    }
}

/*
 * Case 2: the color of the parent of the node is black.
 *
 * \see http://en.wikipedia.org/wiki/Red%E2%80%93black_tree
 *
 * Nothing to do.
 */
void core_red_black_tree_insert_case2(struct core_red_black_tree *self,
                struct core_red_black_node *node)
{
    if (core_red_black_node_is_black(node->parent)) {
        return;
    } else {

        core_red_black_tree_insert_case3(self, node);
    }
}

/*
 * Case 3: node is red, parent is red, grandparent is black and uncle is red.
 *
 * \see http://en.wikipedia.org/wiki/Red%E2%80%93black_tree
 */
void core_red_black_tree_insert_case3(struct core_red_black_tree *self,
                struct core_red_black_node *node)
{
    struct core_red_black_node *parent;
    struct core_red_black_node *grandparent;
    struct core_red_black_node *uncle;

    uncle = core_red_black_node_uncle(node);

    if (!core_red_black_node_is_leaf(uncle)
                    && core_red_black_node_is_red(uncle)) {
        parent = core_red_black_node_parent(node);

        CORE_DEBUGGER_ASSERT(core_red_black_node_is_red(node));

        parent->color = CORE_COLOR_BLACK;
        uncle->color = CORE_COLOR_BLACK;
        grandparent = core_red_black_node_grandparent(node);

        CORE_DEBUGGER_ASSERT(core_red_black_node_is_black(grandparent));

        grandparent->color = CORE_COLOR_RED;

        core_red_black_tree_insert_case1(self, grandparent);
    } else {
        core_red_black_tree_insert_case4(self, node);
    }
}

/*
 * Case 4: the parent is red, but the uncle is black
 *
 * \see http://en.wikipedia.org/wiki/Red%E2%80%93black_tree
 */
void core_red_black_tree_insert_case4(struct core_red_black_tree *self,
                struct core_red_black_node *node)
{
    struct core_red_black_node *grandparent;
    struct core_red_black_node *parent;

#if DEBUG_TREE
    printf("insert_case4 node %d\n", node->key);
    core_red_black_tree_print(self);
#endif

    grandparent = core_red_black_node_grandparent(node);
    parent = core_red_black_node_parent(node);

    if (node == parent->right_node && parent == grandparent->left_node) {

        core_red_black_tree_rotate_left(self, parent);

#ifdef DEBUG_TREE
        printf("insert_case4 rotate_left %d\n", parent->key);
#endif

        node = node->left_node;

    } else if (node == parent->left_node && parent == grandparent->right_node) {
        core_red_black_tree_rotate_right(self, parent);

        node = node->right_node;
    }

#ifdef DEBUG_TREE
    printf("Before call to insert_case5\n");
    core_red_black_tree_print(self);
#endif
    core_red_black_tree_insert_case5(self, node);
}

/*
 * Case 5.
 *
 * \see http://en.wikipedia.org/wiki/Red%E2%80%93black_tree
 */
void core_red_black_tree_insert_case5(struct core_red_black_tree *self,
                struct core_red_black_node *node)
{
    struct core_red_black_node *grandparent;
    struct core_red_black_node *parent;

#ifdef DEBUG_TREE
    printf("insert_case5 %d\n", node->key);
#endif

    grandparent = core_red_black_node_grandparent(node);
    parent = core_red_black_node_parent(node);

    parent->color = CORE_COLOR_BLACK;
    grandparent->color = CORE_COLOR_RED;

    if (node == parent->left_node) {
        core_red_black_tree_rotate_right(self, grandparent);
    } else {
        core_red_black_tree_rotate_left(self, grandparent);
    }
}

/*
 * Left rotation around N (order: C N E D F G)
 *
 *          G
 *      N
 *    C   D
 *       E F
 *
 * (N can also be the right_node of G).
 *
 * becomes
 *
 *          G
 *      D
 *    N   F
 *   C E
 *
 * Changes:
 */
void core_red_black_tree_rotate_left(struct core_red_black_tree *self,
                struct core_red_black_node *node)
{
    struct core_red_black_node *node_N;
    struct core_red_black_node *node_G;
    struct core_red_black_node *node_D;
    struct core_red_black_node *node_E;

#ifdef DEBUG_TREE
    printf("before rotate_left rotate_left node %d\n", node->key);
    core_red_black_tree_print(self);
#endif

    /*
     * List:
     *
     * C -> N [x]
     * N -> E [x]
     */
    node_N = node;
    node_G = node_N->parent;
    node_D = node_N->right_node;
    node_E = node_D->left_node;

    node_N->right_node = node_E;
    if (node_E != NULL) {
        node_E->parent = node_N;
    }

    node_D->left_node = node_N;
    node_N->parent = node_D;

    if (node_G != NULL) {
        if (node_G->left_node == node_N)
            node_G->left_node = node_D;
        else
            node_G->right_node = node_D;
    } else {
        self->root = node_D;
    }
    node_D->parent = node_G;

#ifdef RUN_TREE_ASSERTIONS
    core_red_black_node_run_assertions(node_D, self);
    core_red_black_node_run_assertions(node_N, self);
    core_red_black_node_run_assertions(node_G, self);
    core_red_black_node_run_assertions(node_E, self);
    core_red_black_node_run_assertions(self->root, self);
#endif
}

/*
 * Right rotation around N (order is: G F D E N C)
 *
 * Before
 *
 *          G
 *              N
 *            D   C
 *           F E
 *
 * (N can also be the left_node of G).
 *
 * After
 *
 *
 *          G
 *             D
 *            F  N
 *              E C
 */
void core_red_black_tree_rotate_right(struct core_red_black_tree *self,
                struct core_red_black_node *node)
{
    struct core_red_black_node *node_N;
    struct core_red_black_node *node_G;
    struct core_red_black_node *node_D;
    struct core_red_black_node *node_E;

#if 0
    printf("before rotate_right node %d\n", node->key);
    core_red_black_tree_print(self);
#endif

    node_N = node;
    node_G = node_N->parent;
    node_D = node_N->left_node;
    node_E = node_D->right_node;

    node_N->left_node = node_E;
    if (node_E != NULL) {
        node_E->parent = node_N;
    }

    node_D->right_node = node_N;
    node_N->parent = node_D;

    if (node_G != NULL) {
        if (node_G->right_node == node_N)
            node_G->right_node = node_D;
        else
            node_G->left_node = node_D;
    } else {
        self->root = node_D;
    }
    node_D->parent = node_G;

#if 0
    printf("Before assertions\n");
    core_red_black_tree_print(self);
#endif
#ifdef RUN_TREE_ASSERTIONS
    core_red_black_node_run_assertions(node_N, self);
    core_red_black_node_run_assertions(node_G, self);
    core_red_black_node_run_assertions(node_D, self);
    core_red_black_node_run_assertions(node_E, self);
    core_red_black_node_run_assertions(self->root, self);
#endif

}

void print_spaces(int depth)
{
    while (depth--) {
        printf("    ");
    }
    printf("-->");
}

void core_red_black_tree_print_node(struct core_red_black_tree *self,
                struct core_red_black_node *node, int depth)
{
    if (node == NULL)
        return;

    core_red_black_node_run_assertions(node, self);

    if (core_red_black_node_is_leaf(node)) {
        print_spaces(depth);
        printf("(NIL, BLACK)\n");
    } else {
        print_spaces(depth);

        if (core_red_black_node_is_red(node)) {
            printf("(%d, RED)\n", core_red_black_node_get_key_as_int(node, self->key_size));
        } else {
            printf("(%d, BLACK)\n", core_red_black_node_get_key_as_int(node, self->key_size));
        }

#if 0
        core_red_black_node_run_assertions(node);
#endif

        core_red_black_tree_print_node(self, node->left_node, depth + 1);
        core_red_black_tree_print_node(self, node->right_node, depth + 1);
    }
}

void core_red_black_tree_print(struct core_red_black_tree *self)
{
    printf("Red-black tree content (%d non-NIL nodes):\n", self->size);
    core_red_black_tree_print_node(self, self->root, 0);
    printf("\n");
}

void *core_red_black_tree_get(struct core_red_black_tree *self, void *key)
{
    struct core_red_black_node *node;

    node = core_red_black_tree_get_node(self, key);

    if (!core_red_black_node_is_leaf(node))
        return node->value;

    return NULL;
}

struct core_red_black_node *core_red_black_tree_get_node(struct core_red_black_tree *self, void *key)
{
    struct core_red_black_node *node;
    int result;

#ifdef CORE_RED_BLACK_TREE_USE_CACHE_LAST
    if (self->cached_last_node != NULL
                    && core_red_black_tree_compare(self, key, self->cached_last_node->key) == 0) {
        return self->cached_last_node;
    }
#endif

    node = self->root;

    while (!core_red_black_node_is_leaf(node)) {

        CORE_DEBUGGER_ASSERT(node->key != NULL);

        result = core_red_black_tree_compare(self, key, node->key);

        if (result < 0) {
            node = node->left_node;
        } else if (result > 0) {
            node = node->right_node;
        } else {
#ifdef CORE_RED_BLACK_TREE_USE_CACHE_LAST
            self->cached_last_node = node;
#endif

            break;
        }
    }

    return node;
}

void *core_red_black_tree_get_lowest_key(struct core_red_black_tree *self)
{
    struct core_red_black_node *node;

    if (self->size == 0)
        return NULL;

#ifdef CORE_RED_BLACK_TREE_USE_CACHE_LOWEST
    if (self->cached_lowest_node != NULL) {

#ifdef CORE_RED_BLACK_TREE_USE_CACHE_LAST
        self->cached_last_node = self->cached_lowest_node;
#endif

        return self->cached_lowest_node->key;
    }
#endif

    node = self->root;

    while (!core_red_black_node_is_leaf(node->left_node)) {
        node = node->left_node;
    }

#ifdef CORE_RED_BLACK_TREE_USE_CACHE_LAST
        self->cached_last_node = node;
#endif
    /*
     * This is the lowest value.
     */
    return node->key;
}

int core_red_black_tree_compare(struct core_red_black_tree *self, void *key1, void *key2)
{
    CORE_DEBUGGER_ASSERT(key1 != NULL);
    CORE_DEBUGGER_ASSERT(key2 != NULL);

    return self->compare(self, key1, key2);
}

int core_red_black_tree_compare_memory_content(struct core_red_black_tree *self, void *key1, void *key2)
{
    int result;

#ifdef DEBUG_COMPARE_MEMORY_CONTENT
    int integer1;
    int integer2;

    integer1 = *(int *)key1;
    integer2 = *(int *)key2;
#endif

    result = memcmp(key1, key2, self->key_size);

#ifdef DEBUG_COMPARE_MEMORY_CONTENT
    printf("DEBUG compare_memory_content integer1 %d integer2 %d result %d\n",
                    integer1, integer2, result);
#endif

    return result;
}

int core_red_black_tree_compare_uint64_t(struct core_red_black_tree *self, void *key1, void *key2)
{
    uint64_t key1_as_uint64_t;
    uint64_t key2_as_uint64_t;

    key1_as_uint64_t = *(uint64_t *)key1;
    key2_as_uint64_t = *(uint64_t *)key2;

    if (key1_as_uint64_t < key2_as_uint64_t) {
        return -1;
    } else if (key1_as_uint64_t > key2_as_uint64_t) {
        return 1;
    }

    return 0;
}

void core_red_black_tree_use_uint64_t_keys(struct core_red_black_tree *self)
{
    self->compare = core_red_black_tree_compare_uint64_t;
}

void core_red_black_tree_run_assertions(struct core_red_black_tree *self)
{
    core_red_black_tree_run_assertions_on_node(self, self->root);
}

void core_red_black_tree_run_assertions_on_node(struct core_red_black_tree *self, struct core_red_black_node *node)
{
    if (node == NULL)
        return;

#if 0
    printf("run_assertions_on_node node %d\n",
                    core_red_black_node_get_key_as_int(node, self->key_size));
#endif

    core_red_black_node_run_assertions(node, self);

    core_red_black_tree_run_assertions_on_node(self, node->left_node);
    core_red_black_tree_run_assertions_on_node(self, node->right_node);
}

/*
 * \see http://en.wikipedia.org/wiki/Red%E2%80%93black_tree#Removal
 */
void core_red_black_tree_delete_one_child(struct core_red_black_tree *self, struct core_red_black_node *node)
{
    struct core_red_black_node *child;
    struct core_red_black_node *other_child;

    CORE_DEBUGGER_ASSERT(node->right_node->parent == node);
    CORE_DEBUGGER_ASSERT(node->left_node->parent == node);

#if 0
    printf("delete_one_child node %d\n",
                    core_red_black_node_get_key_as_int(node, self->key_size));
    core_red_black_tree_print(self);
#endif

    /*
     * Pick the non-leaf child, if any.
     */
    child = node->right_node;
    if (core_red_black_node_is_leaf(node->right_node))
        child = node->left_node;

    other_child = core_red_black_node_sibling(child);

#if 0
    printf("Child \n");
    core_red_black_tree_print_node(self, child, 0);
    printf("Other child\n");
    core_red_black_tree_print_node(self, other_child, 0);
#endif

#if 0
    printf("Showing node, child, and other_child\n");
    core_red_black_node_print(node, self->key_size);
    core_red_black_node_print(child, self->key_size);
    core_red_black_node_print(other_child, self->key_size);
    printf("Done.\n");
#endif

    core_red_black_tree_replace_node(self, node, child);

#if 0
    printf("After replace_node\n");
    core_red_black_tree_print(self);
#endif

    if (core_red_black_node_is_black(node)) {
        if (core_red_black_node_is_red(child)) {
            child->color = CORE_COLOR_BLACK;
        } else {
            core_red_black_tree_delete_case1(self, child);
        }
    }

    core_red_black_tree_free_normal_node(self, node);

    /*
     * Destroy the other child if it is a leaf since it is not
     * used anymore.
     */
    if (core_red_black_node_is_leaf(other_child)) {
        core_red_black_tree_free_nil_node(self, other_child);
    }
}

/*
 * \see http://en.wikipedia.org/wiki/Red%E2%80%93black_tree#Removal
 */
void core_red_black_tree_replace_node(struct core_red_black_tree *self, struct core_red_black_node *node,
            struct core_red_black_node *child)
{
#if 0
    printf("In replace_node\n");
    core_red_black_tree_print(self);
#endif

    if (core_red_black_node_is_root(node)) {
        self->root = child;
        child->parent = NULL;

        return;
    }

    if (core_red_black_node_is_left_node(node)) {
        node->parent->left_node = child;
        child->parent = node->parent;
    } else {
        node->parent->right_node = child;
        child->parent = node->parent;
    }
}

/*
 * Case 1: if node N is the root, there is nothing else to do.
 *
 * \see http://en.wikipedia.org/wiki/Red%E2%80%93black_tree#Removal
 */
void core_red_black_tree_delete_case1(struct core_red_black_tree *self, struct core_red_black_node *node)
{
    if (node->parent != NULL)
        core_red_black_tree_delete_case2(self, node);
}

/*
 * \see http://en.wikipedia.org/wiki/Red%E2%80%93black_tree#Removal
 */
void core_red_black_tree_delete_case2(struct core_red_black_tree *self, struct core_red_black_node *node)
{
    struct core_red_black_node *sibling;

    sibling = core_red_black_node_sibling(node);

    if (sibling->color == CORE_COLOR_RED) {
        node->parent->color = CORE_COLOR_RED;
        sibling->color = CORE_COLOR_BLACK;

        if (node == node->parent->left_node)
            core_red_black_tree_rotate_left(self, node->parent);
        else
            core_red_black_tree_rotate_right(self, node->parent);
    }

    core_red_black_tree_delete_case3(self, node);
}

/*
 * \see http://en.wikipedia.org/wiki/Red%E2%80%93black_tree#Removal
 */
void core_red_black_tree_delete_case3(struct core_red_black_tree *self, struct core_red_black_node *node)
{
    struct core_red_black_node *sibling;

    sibling = core_red_black_node_sibling(node);

    if (core_red_black_node_is_black(node->parent)
                    && core_red_black_node_is_black(sibling)
                    && core_red_black_node_is_black(sibling->left_node)
                    && core_red_black_node_is_black(sibling->right_node)) {

        sibling->color = CORE_COLOR_RED;

        core_red_black_tree_delete_case1(self, node->parent);

    } else {

        core_red_black_tree_delete_case4(self, node);
    }
}

/*
 * \see http://en.wikipedia.org/wiki/Red%E2%80%93black_tree#Removal
 */
void core_red_black_tree_delete_case4(struct core_red_black_tree *self, struct core_red_black_node *node)
{
    struct core_red_black_node *sibling;

    sibling = core_red_black_node_sibling(node);

    if (core_red_black_node_is_red(node->parent)
                    && core_red_black_node_is_black(sibling)
                    && core_red_black_node_is_black(sibling->left_node)
                    && core_red_black_node_is_black(sibling->right_node)) {

        sibling->color = CORE_COLOR_RED;
        node->parent->color = CORE_COLOR_BLACK;

    } else {
        core_red_black_tree_delete_case5(self, node);
    }
}

/*
 * \see http://en.wikipedia.org/wiki/Red%E2%80%93black_tree#Removal
 */
void core_red_black_tree_delete_case5(struct core_red_black_tree *self, struct core_red_black_node *node)
{
    struct core_red_black_node *sibling;

    sibling = core_red_black_node_sibling(node);

    if (core_red_black_node_is_black(sibling)) {
        if (core_red_black_node_is_left_node(node)
                        && core_red_black_node_is_black(sibling->right_node)
                        && core_red_black_node_is_red(sibling->left_node)) {

            sibling->color = CORE_COLOR_RED;
            sibling->left_node->color = CORE_COLOR_BLACK;

            core_red_black_tree_rotate_right(self, sibling);
        } else if (core_red_black_node_is_right_node(node)
                        && core_red_black_node_is_black(sibling->left_node)
                        && core_red_black_node_is_red(sibling->right_node)) {

            sibling->color = CORE_COLOR_RED;
            sibling->right_node->color = CORE_COLOR_BLACK;

            core_red_black_tree_rotate_left(self, sibling);
        }
    }

    core_red_black_tree_delete_case6(self, node);
}

/*
 * \see http://en.wikipedia.org/wiki/Red%E2%80%93black_tree#Removal
 */
void core_red_black_tree_delete_case6(struct core_red_black_tree *self, struct core_red_black_node *node)
{
    struct core_red_black_node *sibling;

    sibling = core_red_black_node_sibling(node);

    sibling->color = node->parent->color;
    node->parent->color = CORE_COLOR_BLACK;

    if (core_red_black_node_is_left_node(node)) {
        sibling->right_node->color = CORE_COLOR_BLACK;
        core_red_black_tree_rotate_left(self, node->parent);

    } else {
        sibling->left_node->color = CORE_COLOR_BLACK;
        core_red_black_tree_rotate_right(self, node->parent);
    }
}

struct core_red_black_node *core_red_black_tree_allocate_normal_node(struct core_red_black_tree *self, void *key, void *value)
{
    struct core_red_black_node *node;
    void *new_key;
    void *new_value;

#ifdef CORE_RED_BLACK_TREE_USE_NORMAL_NODE_LIST
    if (self->normal_node_list != NULL) {
        node = self->normal_node_list;
        self->normal_node_list = self->normal_node_list->left_node;
        new_key = node->key;
        new_value = node->value;
    } else {
#endif
        node = core_memory_pool_allocate(self->memory_pool, sizeof(struct core_red_black_node));
        new_key = core_memory_pool_allocate(self->memory_pool, self->key_size);
        new_value = core_memory_pool_allocate(self->memory_pool, self->value_size);

#ifdef CORE_RED_BLACK_TREE_USE_NORMAL_NODE_LIST
    }
#endif

    core_red_black_node_init(node, new_key, new_value);

    core_memory_copy(node->key, key, self->key_size);
    if (value != NULL)
        core_memory_copy(node->value, value, self->value_size);

    return node;
}

void core_red_black_tree_free_normal_node(struct core_red_black_tree *self, struct core_red_black_node *node)
{
    CORE_DEBUGGER_ASSERT(node->key != NULL);
    CORE_DEBUGGER_ASSERT(node->value != NULL);

#ifdef CORE_RED_BLACK_TREE_USE_NORMAL_NODE_LIST
    node->left_node = self->normal_node_list;
    self->normal_node_list = node;
#else
    core_memory_pool_free(self->memory_pool, node->key);
    core_memory_pool_free(self->memory_pool, node->value);
    core_red_black_node_destroy(node);
    core_memory_pool_free(self->memory_pool, node);
#endif
}

struct core_red_black_node *core_red_black_tree_allocate_nil_node(struct core_red_black_tree *self)
{
    struct core_red_black_node *node;

#ifdef CORE_RED_BLACK_TREE_USE_NIL_NODE_LIST
    if (self->nil_node_list != NULL) {
        node = self->nil_node_list;
        self->nil_node_list = self->nil_node_list->left_node;
    } else {
#endif
        node = core_memory_pool_allocate(self->memory_pool, sizeof(struct core_red_black_node));

#ifdef CORE_RED_BLACK_TREE_USE_NIL_NODE_LIST
    }
#endif

    core_red_black_node_init(node, NULL, NULL);

    return node;
}

void core_red_black_tree_free_nil_node(struct core_red_black_tree *self, struct core_red_black_node *node)
{
#ifdef CORE_RED_BLACK_TREE_USE_NIL_NODE_LIST
    node->left_node = self->nil_node_list;
    self->nil_node_list = node;

#else
    core_red_black_node_destroy(node);
    core_memory_pool_free(self->memory_pool, node);
#endif
}
