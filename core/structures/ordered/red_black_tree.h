
#ifndef BIOSAL_RED_BLACK_TREE_H
#define BIOSAL_RED_BLACK_TREE_H

struct biosal_red_black_tree;
struct biosal_red_black_node;
struct biosal_memory_pool;

/*
 * Use a cache to go faster.
 */
#define BIOSAL_RED_BLACK_TREE_USE_CACHE_LAST

/*
 * Use a cache for the node with the lowest key.
 *
 * This is broken and disabled.
 *
 * The following line is incorrect:
 *
 *   self->cached_lowest_node = self->cached_lowest_node->parent
 */
/*
#define BIOSAL_RED_BLACK_TREE_USE_CACHE_LOWEST
*/

/*
*/
#define BIOSAL_RED_BLACK_TREE_USE_NORMAL_NODE_LIST
#define BIOSAL_RED_BLACK_TREE_USE_NIL_NODE_LIST

/*
 * A red-black tree.
 *
 * The 5 algorithmic rules are:
 *
 * 1. A node is red or black.
 * 2. The root is black.
 * 3. All leaf nodes are black.
 * 4. Any red node has 2 black child nodes.
 * 5. Every path from given node to any of its descendent leaf node contains
 *    the same number of black nodes.
 *
 * The implementation is based on pseudo-code in the Wikipedia page.
 *
 * This implementation uses explicit leaf nodes (NIL nodes)
 * because the code is simpler  that way.
 *
 * \see http://en.wikipedia.org/wiki/Red%E2%80%93black_tree
 */
struct biosal_red_black_tree {
    struct biosal_red_black_node *root;
    struct biosal_memory_pool *memory_pool;
    int size;

    int key_size;
    int value_size;

#ifdef BIOSAL_RED_BLACK_TREE_USE_CACHE_LAST
    struct biosal_red_black_node *cached_last_node;
#endif
#ifdef BIOSAL_RED_BLACK_TREE_USE_CACHE_LOWEST
    struct biosal_red_black_node *cached_lowest_node;
#endif

    int (*compare)(struct biosal_red_black_tree *self, void *key1, void *key2);

#ifdef BIOSAL_RED_BLACK_TREE_USE_NIL_NODE_LIST
    struct biosal_red_black_node *nil_node_list;
#endif
#ifdef BIOSAL_RED_BLACK_TREE_USE_NORMAL_NODE_LIST
    struct biosal_red_black_node *normal_node_list;
#endif
};

void biosal_red_black_tree_init(struct biosal_red_black_tree *self, int key_size, int value_size,
                struct biosal_memory_pool *pool);
void biosal_red_black_tree_destroy(struct biosal_red_black_tree *self);

/*
 * Add a key and return a pointer to the uninitialized
 * bucket for the corresponding value.
 *
 * Time complexity: O(log(N))
 */
void *biosal_red_black_tree_add(struct biosal_red_black_tree *self, void *key);

/*
 * Add a key with a value and return a pointer to the initialized
 * bucket for the corresponding value.
 *
 * Time complexity: O(log(N))
 */
void *biosal_red_black_tree_add_key_and_value(struct biosal_red_black_tree *self, void *key, void *value);

/*
 * Delete a key-value pair using the key as search pattern.
 *
 * Time complexity: O(log(N))
 */
void biosal_red_black_tree_delete(struct biosal_red_black_tree *self, void *key);

/*
 * Get the value for a given key. If there are duplicates, return the first match.
 *
 * Time complexity: O(log(N))
 */
void *biosal_red_black_tree_get(struct biosal_red_black_tree *self, void *key);

/*
 * Return the lowest key.
 *
 * Time complexity: O(1)
 */
void *biosal_red_black_tree_get_lowest_key(struct biosal_red_black_tree *self);

/*
 * Check the 5 red-black tree rules.
 *
 * If there is a problem, a non-zero value is returned.
 */
int biosal_red_black_tree_has_ignored_rules(struct biosal_red_black_tree *self);
void biosal_red_black_tree_set_memory_pool(struct biosal_red_black_tree *self,
                struct biosal_memory_pool *memory_pool);

/*
 * Get the number of non-NIL nodes.
 *
 * Time complexity: O(1)
 */
int biosal_red_black_tree_size(struct biosal_red_black_tree *self);

void biosal_red_black_tree_free_node(struct biosal_red_black_tree *self,
                struct biosal_red_black_node *node);

/*
 * 5 cases for insertion, based on the Wikipedia article.
 * \see http://en.wikipedia.org/wiki/Red%E2%80%93black_tree
 */

void biosal_red_black_tree_insert_case1(struct biosal_red_black_tree *self,
                struct biosal_red_black_node *node);
void biosal_red_black_tree_insert_case2(struct biosal_red_black_tree *self,
                struct biosal_red_black_node *node);
void biosal_red_black_tree_insert_case3(struct biosal_red_black_tree *self,
                struct biosal_red_black_node *node);
void biosal_red_black_tree_insert_case4(struct biosal_red_black_tree *self,
                struct biosal_red_black_node *node);
void biosal_red_black_tree_insert_case5(struct biosal_red_black_tree *self,
                struct biosal_red_black_node *node);

void biosal_red_black_tree_rotate_left(struct biosal_red_black_tree *self,
                struct biosal_red_black_node *node);
void biosal_red_black_tree_rotate_right(struct biosal_red_black_tree *self,
                struct biosal_red_black_node *node);

void biosal_red_black_tree_print(struct biosal_red_black_tree *self);
void biosal_red_black_tree_print_node(struct biosal_red_black_tree *self,
                struct biosal_red_black_node *node, int depth);

int biosal_red_black_tree_compare(struct biosal_red_black_tree *self, void *key1, void *key2);
int biosal_red_black_tree_compare_memory_content(struct biosal_red_black_tree *self, void *key1, void *key2);
int biosal_red_black_tree_compare_uint64_t(struct biosal_red_black_tree *self, void *key1, void *key2);
void biosal_red_black_tree_use_uint64_t_keys(struct biosal_red_black_tree *self);
void biosal_red_black_tree_run_assertions(struct biosal_red_black_tree *self);
void biosal_red_black_tree_run_assertions_on_node(struct biosal_red_black_tree *self, struct biosal_red_black_node *node);

void biosal_red_black_tree_delete_one_child(struct biosal_red_black_tree *self, struct biosal_red_black_node *node);
void biosal_red_black_tree_replace_node(struct biosal_red_black_tree *self, struct biosal_red_black_node *node,
                struct biosal_red_black_node *child);
void biosal_red_black_tree_delete_case1(struct biosal_red_black_tree *self, struct biosal_red_black_node *node);
void biosal_red_black_tree_delete_case2(struct biosal_red_black_tree *self, struct biosal_red_black_node *node);
void biosal_red_black_tree_delete_case3(struct biosal_red_black_tree *self, struct biosal_red_black_node *node);
void biosal_red_black_tree_delete_case4(struct biosal_red_black_tree *self, struct biosal_red_black_node *node);
void biosal_red_black_tree_delete_case5(struct biosal_red_black_tree *self, struct biosal_red_black_node *node);
void biosal_red_black_tree_delete_case6(struct biosal_red_black_tree *self, struct biosal_red_black_node *node);

struct biosal_red_black_node *biosal_red_black_tree_get_node(struct biosal_red_black_tree *self, void *key);

void biosal_red_black_tree_delete_private(struct biosal_red_black_tree *self,
                struct biosal_red_black_node *node);

struct biosal_red_black_node *biosal_red_black_tree_allocate_normal_node(struct biosal_red_black_tree *self, void *key, void *value);
void biosal_red_black_tree_free_normal_node(struct biosal_red_black_tree *self, struct biosal_red_black_node *node);
struct biosal_red_black_node *biosal_red_black_tree_allocate_nil_node(struct biosal_red_black_tree *self);
void biosal_red_black_tree_free_nil_node(struct biosal_red_black_tree *self, struct biosal_red_black_node *node);

#endif
