
#include "red_black_tree.h"
#include "red_black_node.h"

#include <core/system/memory_pool.h>
#include <core/system/debugger.h>

#include <stdlib.h>
#include <string.h>

/*
#define RUN_TREE_ASSERTIONS
*/

void biosal_red_black_tree_init(struct biosal_red_black_tree *self, int key_size, int value_size,
                struct biosal_memory_pool *memory_pool)
{
    self->memory_pool = memory_pool;
    self->size = 0;

    self->key_size = key_size;
    self->value_size = value_size;

    self->compare = biosal_red_black_tree_compare_memory_content;

#ifdef BIOSAL_RED_BLACK_TREE_USE_CACHE_LAST
    self->cached_last_node = NULL;
#endif
#ifdef BIOSAL_RED_BLACK_TREE_USE_CACHE_LOWEST
    self->cached_lowest_node = NULL;
#endif

#ifdef BIOSAL_RED_BLACK_TREE_USE_NIL_NODE_LIST
    self->nil_node_list = NULL;
#endif

#ifdef BIOSAL_RED_BLACK_TREE_USE_NORMAL_NODE_LIST
    self->normal_node_list = NULL;
#endif

    self->root = biosal_red_black_tree_allocate_nil_node(self);
}

void biosal_red_black_tree_destroy(struct biosal_red_black_tree *self)
{
#ifdef BIOSAL_RED_BLACK_TREE_USE_NIL_NODE_LIST
    struct biosal_red_black_node *node;
#endif

    if (self->root != NULL) {
        biosal_red_black_tree_free_node(self, self->root);
        self->root = NULL;
    }

#ifdef BIOSAL_RED_BLACK_TREE_USE_NIL_NODE_LIST
    while (self->nil_node_list != NULL) {
        node = self->nil_node_list->left_node;
        biosal_memory_pool_free(self->memory_pool, self->nil_node_list);
        self->nil_node_list = node;
    }
#endif

#ifdef BIOSAL_RED_BLACK_TREE_USE_NORMAL_NODE_LIST
    while (self->normal_node_list != NULL) {
        node = self->normal_node_list->left_node;
        biosal_memory_pool_free(self->memory_pool, self->normal_node_list->key);
        biosal_memory_pool_free(self->memory_pool, self->normal_node_list->value);
        biosal_memory_pool_free(self->memory_pool, self->normal_node_list);
        self->normal_node_list = node;
    }
#endif

    self->memory_pool = NULL;
    self->size = 0;
    self->key_size = 0;
    self->value_size = 0;
}

void *biosal_red_black_tree_add_key_and_value(struct biosal_red_black_tree *self, void *key, void *value)
{
    void *new_value;

    new_value = biosal_red_black_tree_add(self, key);

    BIOSAL_DEBUGGER_ASSERT(new_value != NULL);

    if (value != NULL)
        biosal_memory_copy(new_value, value, self->value_size);

    return new_value;
}

void *biosal_red_black_tree_add(struct biosal_red_black_tree *self, void *key)
{
    struct biosal_red_black_node *node;
    struct biosal_red_black_node *left_nil;
    struct biosal_red_black_node *right_nil;
    struct biosal_red_black_node *current_node;
    struct biosal_red_black_node *left_node;
    struct biosal_red_black_node *right_node;
    int result;
    int inserted;

    BIOSAL_DEBUGGER_ASSERT(self->root != NULL);

    node = biosal_red_black_tree_allocate_normal_node(self, key, NULL);

    BIOSAL_DEBUGGER_ASSERT_NOT_NULL(node->value);

#ifdef BIOSAL_RED_BLACK_TREE_USE_CACHE_LAST
    self->cached_last_node = node;
#endif

    if (biosal_red_black_node_is_leaf(self->root)) {

        left_nil = self->root;
        right_nil = biosal_red_black_tree_allocate_nil_node(self);

        self->root = node;

        biosal_red_black_node_set_left_node(node, left_nil);
        biosal_red_black_node_set_right_node(node, right_nil);

        biosal_red_black_tree_insert_case1(self, node);

#ifdef BIOSAL_RED_BLACK_TREE_USE_CACHE_LOWEST
        /*
         * This is the first item.
         * So this is the lowest key.
         */
        self->cached_lowest_node = node;
#endif

        ++self->size;

        BIOSAL_DEBUGGER_ASSERT_NOT_NULL(node->value);

        return node->value;
    }

    current_node = self->root;
    inserted = 0;

    while (inserted == 0) {

        result = biosal_red_black_tree_compare(self, node->key, current_node->key);

        if (result < 0) {

            left_node = biosal_red_black_node_left_node(current_node);

            if (biosal_red_black_node_is_leaf(left_node)) {

                left_nil = left_node;
                right_nil = biosal_red_black_tree_allocate_nil_node(self);

                biosal_red_black_node_set_left_node(current_node, node);
                biosal_red_black_node_set_left_node(node, left_nil);
                biosal_red_black_node_set_right_node(node, right_nil);

                ++self->size;
                biosal_red_black_tree_insert_case1(self, node);
                inserted = 1;
            } else {
                current_node = left_node;
            }
        } else /* if (biosal_red_black_node_key(node) < biosal_red_black_node_key(current_node)) */ {

            right_node = biosal_red_black_node_right_node(current_node);

            if (biosal_red_black_node_is_leaf(right_node)) {

                left_nil = biosal_red_black_tree_allocate_nil_node(self);
                right_nil = right_node;

                biosal_red_black_node_set_right_node(current_node, node);
                biosal_red_black_node_set_left_node(node, left_nil);
                biosal_red_black_node_set_right_node(node, right_nil);

                ++self->size;
                biosal_red_black_tree_insert_case1(self, node);
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
    if (biosal_red_black_node_is_red(node)) {
        BIOSAL_DEBUGGER_ASSERT(biosal_red_black_node_is_black(node->parent));
    }

    biosal_red_black_node_run_assertions(node, self);
#endif

#ifdef BIOSAL_RED_BLACK_TREE_USE_CACHE_LOWEST
    /*
     * Maintain the cache for the lowest key.
     * To do so, we just need to check if the inserted key is lower than
     * the key for cached_lowest_node.
     *
     * This is O(1).
     */

    if (self->cached_lowest_node == NULL
              || biosal_red_black_tree_compare(self, node->key, self->cached_lowest_node->key) < 0) {

        self->cached_lowest_node = node;

        BIOSAL_DEBUGGER_ASSERT(self->cached_lowest_node->key != NULL);
    }
#endif

    BIOSAL_DEBUGGER_ASSERT_NOT_NULL(node->value);

    return node->value;
}

void biosal_red_black_tree_delete(struct biosal_red_black_tree *self, void *key)
{
    struct biosal_red_black_node *node;

    /*
     * Nothing to delete.
     */
    if (self->size == 0) {
        return;
    }

    /* Find the node
     */
    node = biosal_red_black_tree_get_node(self, key);

    /*
     * The node is not in the tree
     */
    if (node == NULL)
        return;

#ifdef BIOSAL_RED_BLACK_TREE_USE_CACHE_LOWEST
    /*
     * Update the lowest value cache.
     */

    BIOSAL_DEBUGGER_ASSERT(self->cached_lowest_node != NULL || self->cached_lowest_node->key != NULL);

    /*
     * If we are deleting the lowest key, the new lowest key
     * is the parent node for this key (which might be NULL).
     */
    if (self->cached_lowest_node != NULL
                    && biosal_red_black_tree_compare(self, key, self->cached_lowest_node->key) == 0) {
        self->cached_lowest_node = self->cached_lowest_node->parent;
    }

#endif

#ifdef BIOSAL_RED_BLACK_TREE_USE_CACHE_LAST
    /*
     * Remove the cached element pointer.
     */
    if (self->cached_last_node != NULL
                    && biosal_red_black_tree_compare(self, key, self->cached_last_node->key) == 0) {

        self->cached_last_node = NULL;
    }
#endif

    biosal_red_black_tree_delete_private(self, node);
}

void biosal_red_black_tree_delete_private(struct biosal_red_black_tree *self,
                struct biosal_red_black_node *node)
{
    struct biosal_red_black_node *largest_value_node;

    /*
     * Find the largest value before the node
     */
    largest_value_node = node->left_node;

    if (!biosal_red_black_node_is_leaf(largest_value_node)) {

        while (!biosal_red_black_node_is_leaf(largest_value_node->right_node)) {
            largest_value_node = largest_value_node->right_node;
        }
    }

#if 0
    printf("Before Delete node %d\n",
                    biosal_red_black_node_get_key_as_int(node, self->key_size));

    biosal_red_black_tree_print(self);
#endif

    /*
     * Delete the node.
     */
    if (biosal_red_black_node_is_leaf(largest_value_node)) {

#if 0
        printf("delete node %d, no largest value found.\n",
                        biosal_red_black_node_get_key_as_int(node, self->key_size));
#endif
        biosal_red_black_tree_delete_one_child(self, node);

    } else {
        /*
         * Replace the key and the value of the node
         */

#if 0
        printf("Largest value node %d\n",
                    biosal_red_black_node_get_key_as_int(largest_value_node, self->key_size));
#endif

        biosal_memory_copy(node->key, largest_value_node->key, self->key_size);
        biosal_memory_copy(node->value, largest_value_node->value, self->value_size);

#if 0
        biosal_red_black_tree_print(self);
#endif
        biosal_red_black_tree_delete_one_child(self, largest_value_node);
    }

    --self->size;

#if 0
    printf("After Delete\n");
    biosal_red_black_tree_print(self);
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
int biosal_red_black_tree_has_ignored_rules(struct biosal_red_black_tree *self)
{
    if (self->root == NULL) {
        return 0;
    }

    if (biosal_red_black_node_color(self->root) != BIOSAL_COLOR_BLACK) {
        printf("Rule 2 is ignored !\n");
        return 1;
    }

    return 0;
}

void biosal_red_black_tree_set_memory_pool(struct biosal_red_black_tree *self,
                struct biosal_memory_pool *memory_pool)
{
    self->memory_pool = memory_pool;
}

int biosal_red_black_tree_size(struct biosal_red_black_tree *self)
{
    return self->size;
}

void biosal_red_black_tree_free_node(struct biosal_red_black_tree *self,
                struct biosal_red_black_node *node)
{
    struct biosal_red_black_node *left_node;
    struct biosal_red_black_node *right_node;

    if (node == NULL) {
        return;
    }

#if 0
    printf("free_node\n");
#endif

    left_node = biosal_red_black_node_left_node(node);
    right_node = biosal_red_black_node_right_node(node);

    biosal_red_black_tree_free_node(self, left_node);
    biosal_red_black_tree_free_node(self, right_node);

    if (node->key != NULL) {
        biosal_memory_pool_free(self->memory_pool, node->key);
        biosal_memory_pool_free(self->memory_pool, node->value);
    }
    biosal_red_black_node_destroy(node);

#if 0
    printf("free %p\n", (void *)node);
#endif
    biosal_memory_pool_free(self->memory_pool, node);
}

/*
 * Case 1: the node is the root.
 *
 * \see http://en.wikipedia.org/wiki/Red%E2%80%93black_tree
 */
void biosal_red_black_tree_insert_case1(struct biosal_red_black_tree *self,
                struct biosal_red_black_node *node)
{
    if (node->parent == NULL) {
        /*
         * Adding at the root adds one black node to all paths.
         */
        biosal_red_black_node_set_color(self->root, BIOSAL_COLOR_BLACK);
    } else {
        biosal_red_black_tree_insert_case2(self, node);
    }
}

/*
 * Case 2: the color of the parent of the node is black.
 *
 * \see http://en.wikipedia.org/wiki/Red%E2%80%93black_tree
 *
 * Nothing to do.
 */
void biosal_red_black_tree_insert_case2(struct biosal_red_black_tree *self,
                struct biosal_red_black_node *node)
{
    if (biosal_red_black_node_is_black(node->parent)) {
        return;
    } else {

        biosal_red_black_tree_insert_case3(self, node);
    }
}

/*
 * Case 3: node is red, parent is red, grandparent is black and uncle is red.
 *
 * \see http://en.wikipedia.org/wiki/Red%E2%80%93black_tree
 */
void biosal_red_black_tree_insert_case3(struct biosal_red_black_tree *self,
                struct biosal_red_black_node *node)
{
    struct biosal_red_black_node *parent;
    struct biosal_red_black_node *grandparent;
    struct biosal_red_black_node *uncle;

    uncle = biosal_red_black_node_uncle(node);

    if (!biosal_red_black_node_is_leaf(uncle)
                    && biosal_red_black_node_is_red(uncle)) {
        parent = biosal_red_black_node_parent(node);

        BIOSAL_DEBUGGER_ASSERT(biosal_red_black_node_is_red(node));

        parent->color = BIOSAL_COLOR_BLACK;
        uncle->color = BIOSAL_COLOR_BLACK;
        grandparent = biosal_red_black_node_grandparent(node);

        BIOSAL_DEBUGGER_ASSERT(biosal_red_black_node_is_black(grandparent));

        grandparent->color = BIOSAL_COLOR_RED;

        biosal_red_black_tree_insert_case1(self, grandparent);
    } else {
        biosal_red_black_tree_insert_case4(self, node);
    }
}

/*
 * Case 4: the parent is red, but the uncle is black
 *
 * \see http://en.wikipedia.org/wiki/Red%E2%80%93black_tree
 */
void biosal_red_black_tree_insert_case4(struct biosal_red_black_tree *self,
                struct biosal_red_black_node *node)
{
    struct biosal_red_black_node *grandparent;
    struct biosal_red_black_node *parent;

#if DEBUG_TREE
    printf("insert_case4 node %d\n", node->key);
    biosal_red_black_tree_print(self);
#endif

    grandparent = biosal_red_black_node_grandparent(node);
    parent = biosal_red_black_node_parent(node);

    if (node == parent->right_node && parent == grandparent->left_node) {

        biosal_red_black_tree_rotate_left(self, parent);

#ifdef DEBUG_TREE
        printf("insert_case4 rotate_left %d\n", parent->key);
#endif

        node = node->left_node;

    } else if (node == parent->left_node && parent == grandparent->right_node) {
        biosal_red_black_tree_rotate_right(self, parent);

        node = node->right_node;
    }

#ifdef DEBUG_TREE
    printf("Before call to insert_case5\n");
    biosal_red_black_tree_print(self);
#endif
    biosal_red_black_tree_insert_case5(self, node);
}

/*
 * Case 5.
 *
 * \see http://en.wikipedia.org/wiki/Red%E2%80%93black_tree
 */
void biosal_red_black_tree_insert_case5(struct biosal_red_black_tree *self,
                struct biosal_red_black_node *node)
{
    struct biosal_red_black_node *grandparent;
    struct biosal_red_black_node *parent;

#ifdef DEBUG_TREE
    printf("insert_case5 %d\n", node->key);
#endif

    grandparent = biosal_red_black_node_grandparent(node);
    parent = biosal_red_black_node_parent(node);

    parent->color = BIOSAL_COLOR_BLACK;
    grandparent->color = BIOSAL_COLOR_RED;

    if (node == parent->left_node) {
        biosal_red_black_tree_rotate_right(self, grandparent);
    } else {
        biosal_red_black_tree_rotate_left(self, grandparent);
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
void biosal_red_black_tree_rotate_left(struct biosal_red_black_tree *self,
                struct biosal_red_black_node *node)
{
    struct biosal_red_black_node *node_N;
    struct biosal_red_black_node *node_G;
    struct biosal_red_black_node *node_D;
    struct biosal_red_black_node *node_E;

#ifdef DEBUG_TREE
    printf("before rotate_left rotate_left node %d\n", node->key);
    biosal_red_black_tree_print(self);
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
    biosal_red_black_node_run_assertions(node_D, self);
    biosal_red_black_node_run_assertions(node_N, self);
    biosal_red_black_node_run_assertions(node_G, self);
    biosal_red_black_node_run_assertions(node_E, self);
    biosal_red_black_node_run_assertions(self->root, self);
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
void biosal_red_black_tree_rotate_right(struct biosal_red_black_tree *self,
                struct biosal_red_black_node *node)
{
    struct biosal_red_black_node *node_N;
    struct biosal_red_black_node *node_G;
    struct biosal_red_black_node *node_D;
    struct biosal_red_black_node *node_E;

#if 0
    printf("before rotate_right node %d\n", node->key);
    biosal_red_black_tree_print(self);
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
    biosal_red_black_tree_print(self);
#endif
#ifdef RUN_TREE_ASSERTIONS
    biosal_red_black_node_run_assertions(node_N, self);
    biosal_red_black_node_run_assertions(node_G, self);
    biosal_red_black_node_run_assertions(node_D, self);
    biosal_red_black_node_run_assertions(node_E, self);
    biosal_red_black_node_run_assertions(self->root, self);
#endif

}

void print_spaces(int depth)
{
    while (depth--) {
        printf("    ");
    }
    printf("-->");
}

void biosal_red_black_tree_print_node(struct biosal_red_black_tree *self,
                struct biosal_red_black_node *node, int depth)
{
    if (node == NULL)
        return;

    biosal_red_black_node_run_assertions(node, self);

    if (biosal_red_black_node_is_leaf(node)) {
        print_spaces(depth);
        printf("(NIL, BLACK)\n");
    } else {
        print_spaces(depth);

        if (biosal_red_black_node_is_red(node)) {
            printf("(%d, RED)\n", biosal_red_black_node_get_key_as_int(node, self->key_size));
        } else {
            printf("(%d, BLACK)\n", biosal_red_black_node_get_key_as_int(node, self->key_size));
        }

#if 0
        biosal_red_black_node_run_assertions(node);
#endif

        biosal_red_black_tree_print_node(self, node->left_node, depth + 1);
        biosal_red_black_tree_print_node(self, node->right_node, depth + 1);
    }
}

void biosal_red_black_tree_print(struct biosal_red_black_tree *self)
{
    printf("Red-black tree content (%d non-NIL nodes):\n", self->size);
    biosal_red_black_tree_print_node(self, self->root, 0);
    printf("\n");
}

void *biosal_red_black_tree_get(struct biosal_red_black_tree *self, void *key)
{
    struct biosal_red_black_node *node;

    node = biosal_red_black_tree_get_node(self, key);

    if (!biosal_red_black_node_is_leaf(node))
        return node->value;

    return NULL;
}

struct biosal_red_black_node *biosal_red_black_tree_get_node(struct biosal_red_black_tree *self, void *key)
{
    struct biosal_red_black_node *node;
    int result;

#ifdef BIOSAL_RED_BLACK_TREE_USE_CACHE_LAST
    if (self->cached_last_node != NULL
                    && biosal_red_black_tree_compare(self, key, self->cached_last_node->key) == 0) {
        return self->cached_last_node;
    }
#endif

    node = self->root;

    while (!biosal_red_black_node_is_leaf(node)) {

        BIOSAL_DEBUGGER_ASSERT(node->key != NULL);

        result = biosal_red_black_tree_compare(self, key, node->key);

        if (result < 0) {
            node = node->left_node;
        } else if (result > 0) {
            node = node->right_node;
        } else {
#ifdef BIOSAL_RED_BLACK_TREE_USE_CACHE_LAST
            self->cached_last_node = node;
#endif

            break;
        }
    }

    return node;
}

void *biosal_red_black_tree_get_lowest_key(struct biosal_red_black_tree *self)
{
    struct biosal_red_black_node *node;

    if (self->size == 0)
        return NULL;

#ifdef BIOSAL_RED_BLACK_TREE_USE_CACHE_LOWEST
    if (self->cached_lowest_node != NULL) {

#ifdef BIOSAL_RED_BLACK_TREE_USE_CACHE_LAST
        self->cached_last_node = self->cached_lowest_node;
#endif

        return self->cached_lowest_node->key;
    }
#endif

    node = self->root;

    while (!biosal_red_black_node_is_leaf(node->left_node)) {
        node = node->left_node;
    }

#ifdef BIOSAL_RED_BLACK_TREE_USE_CACHE_LAST
        self->cached_last_node = node;
#endif
    /*
     * This is the lowest value.
     */
    return node->key;
}

int biosal_red_black_tree_compare(struct biosal_red_black_tree *self, void *key1, void *key2)
{
    BIOSAL_DEBUGGER_ASSERT(key1 != NULL);
    BIOSAL_DEBUGGER_ASSERT(key2 != NULL);

    return self->compare(self, key1, key2);
}

int biosal_red_black_tree_compare_memory_content(struct biosal_red_black_tree *self, void *key1, void *key2)
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

int biosal_red_black_tree_compare_uint64_t(struct biosal_red_black_tree *self, void *key1, void *key2)
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

void biosal_red_black_tree_use_uint64_t_keys(struct biosal_red_black_tree *self)
{
    self->compare = biosal_red_black_tree_compare_uint64_t;
}

void biosal_red_black_tree_run_assertions(struct biosal_red_black_tree *self)
{
    biosal_red_black_tree_run_assertions_on_node(self, self->root);
}

void biosal_red_black_tree_run_assertions_on_node(struct biosal_red_black_tree *self, struct biosal_red_black_node *node)
{
    if (node == NULL)
        return;

#if 0
    printf("run_assertions_on_node node %d\n",
                    biosal_red_black_node_get_key_as_int(node, self->key_size));
#endif

    biosal_red_black_node_run_assertions(node, self);

    biosal_red_black_tree_run_assertions_on_node(self, node->left_node);
    biosal_red_black_tree_run_assertions_on_node(self, node->right_node);
}

/*
 * \see http://en.wikipedia.org/wiki/Red%E2%80%93black_tree#Removal
 */
void biosal_red_black_tree_delete_one_child(struct biosal_red_black_tree *self, struct biosal_red_black_node *node)
{
    struct biosal_red_black_node *child;
    struct biosal_red_black_node *other_child;

    BIOSAL_DEBUGGER_ASSERT(node->right_node->parent == node);
    BIOSAL_DEBUGGER_ASSERT(node->left_node->parent == node);

#if 0
    printf("delete_one_child node %d\n",
                    biosal_red_black_node_get_key_as_int(node, self->key_size));
    biosal_red_black_tree_print(self);
#endif

    /*
     * Pick the non-leaf child, if any.
     */
    child = node->right_node;
    if (biosal_red_black_node_is_leaf(node->right_node))
        child = node->left_node;

    other_child = biosal_red_black_node_sibling(child);

#if 0
    printf("Child \n");
    biosal_red_black_tree_print_node(self, child, 0);
    printf("Other child\n");
    biosal_red_black_tree_print_node(self, other_child, 0);
#endif

#if 0
    printf("Showing node, child, and other_child\n");
    biosal_red_black_node_print(node, self->key_size);
    biosal_red_black_node_print(child, self->key_size);
    biosal_red_black_node_print(other_child, self->key_size);
    printf("Done.\n");
#endif

    biosal_red_black_tree_replace_node(self, node, child);

#if 0
    printf("After replace_node\n");
    biosal_red_black_tree_print(self);
#endif

    if (biosal_red_black_node_is_black(node)) {
        if (biosal_red_black_node_is_red(child)) {
            child->color = BIOSAL_COLOR_BLACK;
        } else {
            biosal_red_black_tree_delete_case1(self, child);
        }
    }

    biosal_red_black_tree_free_normal_node(self, node);

    /*
     * Destroy the other child if it is a leaf since it is not
     * used anymore.
     */
    if (biosal_red_black_node_is_leaf(other_child)) {
        biosal_red_black_tree_free_nil_node(self, other_child);
    }
}

/*
 * \see http://en.wikipedia.org/wiki/Red%E2%80%93black_tree#Removal
 */
void biosal_red_black_tree_replace_node(struct biosal_red_black_tree *self, struct biosal_red_black_node *node,
            struct biosal_red_black_node *child)
{
#if 0
    printf("In replace_node\n");
    biosal_red_black_tree_print(self);
#endif

    if (biosal_red_black_node_is_root(node)) {
        self->root = child;
        child->parent = NULL;

        return;
    }

    if (biosal_red_black_node_is_left_node(node)) {
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
void biosal_red_black_tree_delete_case1(struct biosal_red_black_tree *self, struct biosal_red_black_node *node)
{
    if (node->parent != NULL)
        biosal_red_black_tree_delete_case2(self, node);
}

/*
 * \see http://en.wikipedia.org/wiki/Red%E2%80%93black_tree#Removal
 */
void biosal_red_black_tree_delete_case2(struct biosal_red_black_tree *self, struct biosal_red_black_node *node)
{
    struct biosal_red_black_node *sibling;

    sibling = biosal_red_black_node_sibling(node);

    if (sibling->color == BIOSAL_COLOR_RED) {
        node->parent->color = BIOSAL_COLOR_RED;
        sibling->color = BIOSAL_COLOR_BLACK;

        if (node == node->parent->left_node)
            biosal_red_black_tree_rotate_left(self, node->parent);
        else
            biosal_red_black_tree_rotate_right(self, node->parent);
    }

    biosal_red_black_tree_delete_case3(self, node);
}

/*
 * \see http://en.wikipedia.org/wiki/Red%E2%80%93black_tree#Removal
 */
void biosal_red_black_tree_delete_case3(struct biosal_red_black_tree *self, struct biosal_red_black_node *node)
{
    struct biosal_red_black_node *sibling;

    sibling = biosal_red_black_node_sibling(node);

    if (biosal_red_black_node_is_black(node->parent)
                    && biosal_red_black_node_is_black(sibling)
                    && biosal_red_black_node_is_black(sibling->left_node)
                    && biosal_red_black_node_is_black(sibling->right_node)) {

        sibling->color = BIOSAL_COLOR_RED;

        biosal_red_black_tree_delete_case1(self, node->parent);

    } else {

        biosal_red_black_tree_delete_case4(self, node);
    }
}

/*
 * \see http://en.wikipedia.org/wiki/Red%E2%80%93black_tree#Removal
 */
void biosal_red_black_tree_delete_case4(struct biosal_red_black_tree *self, struct biosal_red_black_node *node)
{
    struct biosal_red_black_node *sibling;

    sibling = biosal_red_black_node_sibling(node);

    if (biosal_red_black_node_is_red(node->parent)
                    && biosal_red_black_node_is_black(sibling)
                    && biosal_red_black_node_is_black(sibling->left_node)
                    && biosal_red_black_node_is_black(sibling->right_node)) {

        sibling->color = BIOSAL_COLOR_RED;
        node->parent->color = BIOSAL_COLOR_BLACK;

    } else {
        biosal_red_black_tree_delete_case5(self, node);
    }
}

/*
 * \see http://en.wikipedia.org/wiki/Red%E2%80%93black_tree#Removal
 */
void biosal_red_black_tree_delete_case5(struct biosal_red_black_tree *self, struct biosal_red_black_node *node)
{
    struct biosal_red_black_node *sibling;

    sibling = biosal_red_black_node_sibling(node);

    if (biosal_red_black_node_is_black(sibling)) {
        if (biosal_red_black_node_is_left_node(node)
                        && biosal_red_black_node_is_black(sibling->right_node)
                        && biosal_red_black_node_is_red(sibling->left_node)) {

            sibling->color = BIOSAL_COLOR_RED;
            sibling->left_node->color = BIOSAL_COLOR_BLACK;

            biosal_red_black_tree_rotate_right(self, sibling);
        } else if (biosal_red_black_node_is_right_node(node)
                        && biosal_red_black_node_is_black(sibling->left_node)
                        && biosal_red_black_node_is_red(sibling->right_node)) {

            sibling->color = BIOSAL_COLOR_RED;
            sibling->right_node->color = BIOSAL_COLOR_BLACK;

            biosal_red_black_tree_rotate_left(self, sibling);
        }
    }

    biosal_red_black_tree_delete_case6(self, node);
}

/*
 * \see http://en.wikipedia.org/wiki/Red%E2%80%93black_tree#Removal
 */
void biosal_red_black_tree_delete_case6(struct biosal_red_black_tree *self, struct biosal_red_black_node *node)
{
    struct biosal_red_black_node *sibling;

    sibling = biosal_red_black_node_sibling(node);

    sibling->color = node->parent->color;
    node->parent->color = BIOSAL_COLOR_BLACK;

    if (biosal_red_black_node_is_left_node(node)) {
        sibling->right_node->color = BIOSAL_COLOR_BLACK;
        biosal_red_black_tree_rotate_left(self, node->parent);

    } else {
        sibling->left_node->color = BIOSAL_COLOR_BLACK;
        biosal_red_black_tree_rotate_right(self, node->parent);
    }
}

struct biosal_red_black_node *biosal_red_black_tree_allocate_normal_node(struct biosal_red_black_tree *self, void *key, void *value)
{
    struct biosal_red_black_node *node;
    void *new_key;
    void *new_value;

#ifdef BIOSAL_RED_BLACK_TREE_USE_NORMAL_NODE_LIST
    if (self->normal_node_list != NULL) {
        node = self->normal_node_list;
        self->normal_node_list = self->normal_node_list->left_node;
        new_key = node->key;
        new_value = node->value;
    } else {
#endif
        node = biosal_memory_pool_allocate(self->memory_pool, sizeof(struct biosal_red_black_node));
        new_key = biosal_memory_pool_allocate(self->memory_pool, self->key_size);
        new_value = biosal_memory_pool_allocate(self->memory_pool, self->value_size);

#ifdef BIOSAL_RED_BLACK_TREE_USE_NORMAL_NODE_LIST
    }
#endif

    biosal_red_black_node_init(node, new_key, new_value);

    biosal_memory_copy(node->key, key, self->key_size);
    if (value != NULL)
        biosal_memory_copy(node->value, value, self->value_size);

    return node;
}

void biosal_red_black_tree_free_normal_node(struct biosal_red_black_tree *self, struct biosal_red_black_node *node)
{
    BIOSAL_DEBUGGER_ASSERT(node->key != NULL);
    BIOSAL_DEBUGGER_ASSERT(node->value != NULL);

#ifdef BIOSAL_RED_BLACK_TREE_USE_NORMAL_NODE_LIST
    node->left_node = self->normal_node_list;
    self->normal_node_list = node;
#else
    biosal_memory_pool_free(self->memory_pool, node->key);
    biosal_memory_pool_free(self->memory_pool, node->value);
    biosal_red_black_node_destroy(node);
    biosal_memory_pool_free(self->memory_pool, node);
#endif
}

struct biosal_red_black_node *biosal_red_black_tree_allocate_nil_node(struct biosal_red_black_tree *self)
{
    struct biosal_red_black_node *node;

#ifdef BIOSAL_RED_BLACK_TREE_USE_NIL_NODE_LIST
    if (self->nil_node_list != NULL) {
        node = self->nil_node_list;
        self->nil_node_list = self->nil_node_list->left_node;
    } else {
#endif
        node = biosal_memory_pool_allocate(self->memory_pool, sizeof(struct biosal_red_black_node));

#ifdef BIOSAL_RED_BLACK_TREE_USE_NIL_NODE_LIST
    }
#endif

    biosal_red_black_node_init(node, NULL, NULL);

    return node;
}

void biosal_red_black_tree_free_nil_node(struct biosal_red_black_tree *self, struct biosal_red_black_node *node)
{
#ifdef BIOSAL_RED_BLACK_TREE_USE_NIL_NODE_LIST
    node->left_node = self->nil_node_list;
    self->nil_node_list = node;

#else
    biosal_red_black_node_destroy(node);
    biosal_memory_pool_free(self->memory_pool, node);
#endif
}
