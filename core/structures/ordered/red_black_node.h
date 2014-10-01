
#ifndef BIOSAL_RED_BLACK_NODE_H
#define BIOSAL_RED_BLACK_NODE_H

#define BIOSAL_COLOR_NONE     0
#define BIOSAL_COLOR_RED      1
#define BIOSAL_COLOR_BLACK    2

struct biosal_red_black_node;
struct biosal_red_black_tree;
struct biosal_memory_pool;

/*
 * A red-black node.
 *
 * Code is based on the content of the Wikipedia article.
 *
 * \see http://en.wikipedia.org/wiki/Red%E2%80%93black_tree
 */
struct biosal_red_black_node {
    struct biosal_red_black_node *parent;
    struct biosal_red_black_node *left_node;
    struct biosal_red_black_node *right_node;
    char color;

    void *key;
    void *value;
};

void biosal_red_black_node_init(struct biosal_red_black_node *self, void *key, void *value);
void biosal_red_black_node_destroy(struct biosal_red_black_node *self);

void *biosal_red_black_node_key(struct biosal_red_black_node *self);
char biosal_red_black_node_color(struct biosal_red_black_node *self);
void biosal_red_black_node_set_color(struct biosal_red_black_node *self, char color);

struct biosal_red_black_node *biosal_red_black_node_left_node(struct biosal_red_black_node *self);
void biosal_red_black_node_set_left_node(struct biosal_red_black_node *self, struct biosal_red_black_node *node);

struct biosal_red_black_node *biosal_red_black_node_right_node(struct biosal_red_black_node *self);
void biosal_red_black_node_set_right_node(struct biosal_red_black_node *self, struct biosal_red_black_node *node);

struct biosal_red_black_node *biosal_red_black_node_parent(struct biosal_red_black_node *self);
void biosal_red_black_node_set_parent(struct biosal_red_black_node *self, struct biosal_red_black_node *node);

struct biosal_red_black_node *biosal_red_black_node_uncle(struct biosal_red_black_node *self);
struct biosal_red_black_node *biosal_red_black_node_grandparent(struct biosal_red_black_node *self);

struct biosal_red_black_node *biosal_red_black_node_sibling(struct biosal_red_black_node *self);

void biosal_red_black_node_run_assertions(struct biosal_red_black_node *self, struct biosal_red_black_tree *tree);

int biosal_red_black_node_get_key_as_int(struct biosal_red_black_node *self, int key_size);

int biosal_red_black_node_is_red(struct biosal_red_black_node *self);
int biosal_red_black_node_is_black(struct biosal_red_black_node *self);
int biosal_red_black_node_is_leaf(struct biosal_red_black_node *self);
int biosal_red_black_node_is_root(struct biosal_red_black_node *self);
int biosal_red_black_node_is_left_node(struct biosal_red_black_node *self);
int biosal_red_black_node_is_right_node(struct biosal_red_black_node *self);

void biosal_red_black_node_print(struct biosal_red_black_node *self, int key_size);

#endif /* BIOSAL_RED_BLACK_NODE_H */
