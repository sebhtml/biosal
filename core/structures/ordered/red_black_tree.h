
#ifndef CORE_RED_BLACK_TREE_H
#define CORE_RED_BLACK_TREE_H

struct core_red_black_tree;
struct core_red_black_node;
struct core_memory_pool;

/*
 * Use a cache to go faster.
 */
#define CORE_RED_BLACK_TREE_USE_CACHE_LAST

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
#define CORE_RED_BLACK_TREE_USE_CACHE_LOWEST
*/

/*
*/
#define CORE_RED_BLACK_TREE_USE_NORMAL_NODE_LIST
#define CORE_RED_BLACK_TREE_USE_NIL_NODE_LIST

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
struct core_red_black_tree {
    struct core_red_black_node *root;
    struct core_memory_pool *memory_pool;
    int size;

    int key_size;
    int value_size;

#ifdef CORE_RED_BLACK_TREE_USE_CACHE_LAST
    struct core_red_black_node *cached_last_node;
#endif
#ifdef CORE_RED_BLACK_TREE_USE_CACHE_LOWEST
    struct core_red_black_node *cached_lowest_node;
#endif

    int (*compare)(struct core_red_black_tree *self, void *key1, void *key2);

#ifdef CORE_RED_BLACK_TREE_USE_NIL_NODE_LIST
    struct core_red_black_node *nil_node_list;
#endif
#ifdef CORE_RED_BLACK_TREE_USE_NORMAL_NODE_LIST
    struct core_red_black_node *normal_node_list;
#endif
};

void core_red_black_tree_init(struct core_red_black_tree *self, int key_size, int value_size,
                struct core_memory_pool *pool);
void core_red_black_tree_destroy(struct core_red_black_tree *self);

/*
 * Add a key and return a pointer to the uninitialized
 * bucket for the corresponding value.
 *
 * Time complexity: O(log(N))
 */
void *core_red_black_tree_add(struct core_red_black_tree *self, void *key);

/*
 * Add a key with a value and return a pointer to the initialized
 * bucket for the corresponding value.
 *
 * Time complexity: O(log(N))
 */
void *core_red_black_tree_add_key_and_value(struct core_red_black_tree *self, void *key, void *value);

/*
 * Delete a key-value pair using the key as search pattern.
 *
 * Time complexity: O(log(N))
 */
void core_red_black_tree_delete(struct core_red_black_tree *self, void *key);

/*
 * Get the value for a given key. If there are duplicates, return the first match.
 *
 * Time complexity: O(log(N))
 */
void *core_red_black_tree_get(struct core_red_black_tree *self, void *key);

/*
 * Return the lowest key.
 *
 * Time complexity: O(1)
 */
void *core_red_black_tree_get_lowest_key(struct core_red_black_tree *self);

void core_red_black_tree_set_memory_pool(struct core_red_black_tree *self,
                struct core_memory_pool *memory_pool);

/*
 * Get the number of non-NIL nodes.
 *
 * Time complexity: O(1)
 */
int core_red_black_tree_size(struct core_red_black_tree *self);
int core_red_black_tree_empty(struct core_red_black_tree *self);

void core_red_black_tree_use_uint64_t_keys(struct core_red_black_tree *self);

int core_red_black_tree_compare(struct core_red_black_tree *self, void *key1, void *key2);

/*
 * Check the 5 red-black tree rules.
 *
 * If there is a problem, a non-zero value is returned.
 */
int core_red_black_tree_has_ignored_rules(struct core_red_black_tree *self);

void core_red_black_tree_run_assertions(struct core_red_black_tree *self);

#endif
