
#ifndef BSAL_RED_BLACK_TREE_H
#define BSAL_RED_BLACK_TREE_H

struct bsal_red_black_tree;
struct bsal_red_black_node;
struct bsal_memory_pool;

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
 * \see http://en.wikipedia.org/wiki/Red%E2%80%93black_tree
 */
struct bsal_red_black_tree {
    struct bsal_red_black_node *root;
    struct bsal_memory_pool *memory_pool;
    int size;

    int key_size;
    int value_size;

    struct bsal_red_black_node *cached_last_result;
    struct bsal_red_black_node *cached_lowest_result;

    int (*compare)(struct bsal_red_black_tree *self, void *key1, void *key2);
};

void bsal_red_black_tree_init(struct bsal_red_black_tree *self, int key_size, int value_size);
void bsal_red_black_tree_destroy(struct bsal_red_black_tree *self);

/*
 * Add a key and return a pointer to the uninitialized
 * bucket for the corresponding value.
 */
void *bsal_red_black_tree_add(struct bsal_red_black_tree *self, void *key);

/*
 * Add a key with a value and return a pointer to the initialized
 * bucket for the corresponding value.
 */
void *bsal_red_black_tree_add_key_and_value(struct bsal_red_black_tree *self, void *key, void *value);

/*
 * Delete a key-value pair using the key as search pattern.
 */
void bsal_red_black_tree_delete(struct bsal_red_black_tree *self, void *key);

/*
 * Get the value for a given key. If there are duplicates, return the first match.
 */
void *bsal_red_black_tree_get(struct bsal_red_black_tree *self, void *key);

/*
 * Return the lowest key.
 */
void *bsal_red_black_tree_get_lowest_key(struct bsal_red_black_tree *self);

/*
 * Check the 5 red-black tree rules.
 *
 * If there is a problem, a non-zero value is returned.
 */
int bsal_red_black_tree_has_ignored_rules(struct bsal_red_black_tree *self);
void bsal_red_black_tree_set_memory_pool(struct bsal_red_black_tree *self,
                struct bsal_memory_pool *memory_pool);

int bsal_red_black_tree_size(struct bsal_red_black_tree *self);

void bsal_red_black_tree_free_node(struct bsal_red_black_tree *self,
                struct bsal_red_black_node *node);

/*
 * 5 cases for insertion, based on the Wikipedia article.
 * \see http://en.wikipedia.org/wiki/Red%E2%80%93black_tree
 */

void bsal_red_black_tree_insert_case1(struct bsal_red_black_tree *self,
                struct bsal_red_black_node *node);
void bsal_red_black_tree_insert_case2(struct bsal_red_black_tree *self,
                struct bsal_red_black_node *node);
void bsal_red_black_tree_insert_case3(struct bsal_red_black_tree *self,
                struct bsal_red_black_node *node);
void bsal_red_black_tree_insert_case4(struct bsal_red_black_tree *self,
                struct bsal_red_black_node *node);
void bsal_red_black_tree_insert_case5(struct bsal_red_black_tree *self,
                struct bsal_red_black_node *node);

void bsal_red_black_tree_rotate_left(struct bsal_red_black_tree *self,
                struct bsal_red_black_node *node);
void bsal_red_black_tree_rotate_right(struct bsal_red_black_tree *self,
                struct bsal_red_black_node *node);

void bsal_red_black_tree_print(struct bsal_red_black_tree *self);
void bsal_red_black_tree_print_node(struct bsal_red_black_tree *self,
                struct bsal_red_black_node *node, int depth);

int bsal_red_black_tree_compare(struct bsal_red_black_tree *self, void *key1, void *key2);
int bsal_red_black_tree_compare_memory_content(struct bsal_red_black_tree *self, void *key1, void *key2);
int bsal_red_black_tree_compare_uint64_t(struct bsal_red_black_tree *self, void *key1, void *key2);
void bsal_red_black_tree_use_uint64_t_keys(struct bsal_red_black_tree *self);
void bsal_red_black_tree_run_assertions(struct bsal_red_black_tree *self);
void bsal_red_black_tree_run_assertions_on_node(struct bsal_red_black_tree *self, struct bsal_red_black_node *node);

#endif
