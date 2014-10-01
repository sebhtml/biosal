
#ifndef CORE_RED_BLACK_NODE_H
#define CORE_RED_BLACK_NODE_H

#define CORE_COLOR_NONE     0
#define CORE_COLOR_RED      1
#define CORE_COLOR_BLACK    2

struct core_red_black_node;
struct core_red_black_tree;
struct core_memory_pool;

/*
 * A red-black node.
 *
 * Code is based on the content of the Wikipedia article.
 *
 * \see http://en.wikipedia.org/wiki/Red%E2%80%93black_tree
 */
struct core_red_black_node {
    struct core_red_black_node *parent;
    struct core_red_black_node *left_node;
    struct core_red_black_node *right_node;
    char color;

    void *key;
    void *value;
};

void core_red_black_node_init(struct core_red_black_node *self, void *key, void *value);
void core_red_black_node_destroy(struct core_red_black_node *self);

void *core_red_black_node_key(struct core_red_black_node *self);
char core_red_black_node_color(struct core_red_black_node *self);
void core_red_black_node_set_color(struct core_red_black_node *self, char color);

struct core_red_black_node *core_red_black_node_left_node(struct core_red_black_node *self);
void core_red_black_node_set_left_node(struct core_red_black_node *self, struct core_red_black_node *node);

struct core_red_black_node *core_red_black_node_right_node(struct core_red_black_node *self);
void core_red_black_node_set_right_node(struct core_red_black_node *self, struct core_red_black_node *node);

struct core_red_black_node *core_red_black_node_parent(struct core_red_black_node *self);
void core_red_black_node_set_parent(struct core_red_black_node *self, struct core_red_black_node *node);

struct core_red_black_node *core_red_black_node_uncle(struct core_red_black_node *self);
struct core_red_black_node *core_red_black_node_grandparent(struct core_red_black_node *self);

struct core_red_black_node *core_red_black_node_sibling(struct core_red_black_node *self);

void core_red_black_node_run_assertions(struct core_red_black_node *self, struct core_red_black_tree *tree);

int core_red_black_node_get_key_as_int(struct core_red_black_node *self, int key_size);

int core_red_black_node_is_red(struct core_red_black_node *self);
int core_red_black_node_is_black(struct core_red_black_node *self);
int core_red_black_node_is_leaf(struct core_red_black_node *self);
int core_red_black_node_is_root(struct core_red_black_node *self);
int core_red_black_node_is_left_node(struct core_red_black_node *self);
int core_red_black_node_is_right_node(struct core_red_black_node *self);

void core_red_black_node_print(struct core_red_black_node *self, int key_size);

#endif /* CORE_RED_BLACK_NODE_H */
