
#ifndef BSAL_RED_BLACK_NODE_H
#define BSAL_RED_BLACK_NODE_H

#define BSAL_COLOR_NONE     0
#define BSAL_COLOR_RED      1
#define BSAL_COLOR_BLACK    2

struct bsal_red_black_node;
struct bsal_red_black_tree;
struct bsal_memory_pool;

/*
 * A red-black node.
 */
struct bsal_red_black_node {
    struct bsal_red_black_node *parent;
    struct bsal_red_black_node *left_node;
    struct bsal_red_black_node *right_node;
    char color;

    void *key;
    void *value;
};

void bsal_red_black_node_init(struct bsal_red_black_node *self, int key_size, void *key,
                int value_size, void *value, struct bsal_memory_pool *pool);
void bsal_red_black_node_destroy(struct bsal_red_black_node *self, struct bsal_memory_pool *pool);

void *bsal_red_black_node_key(struct bsal_red_black_node *self);
char bsal_red_black_node_color(struct bsal_red_black_node *self);
void bsal_red_black_node_set_color(struct bsal_red_black_node *self, char color);

struct bsal_red_black_node *bsal_red_black_node_left_node(struct bsal_red_black_node *self);
void bsal_red_black_node_set_left_node(struct bsal_red_black_node *self, struct bsal_red_black_node *node);

struct bsal_red_black_node *bsal_red_black_node_right_node(struct bsal_red_black_node *self);
void bsal_red_black_node_set_right_node(struct bsal_red_black_node *self, struct bsal_red_black_node *node);

struct bsal_red_black_node *bsal_red_black_node_parent(struct bsal_red_black_node *self);
void bsal_red_black_node_set_parent(struct bsal_red_black_node *self, struct bsal_red_black_node *node);

struct bsal_red_black_node *bsal_red_black_node_uncle(struct bsal_red_black_node *self);
struct bsal_red_black_node *bsal_red_black_node_grandparent(struct bsal_red_black_node *self);

void bsal_red_black_node_run_assertions(struct bsal_red_black_node *self, struct bsal_red_black_tree *tree);

int bsal_red_black_node_get_key_as_int(struct bsal_red_black_node *self, int key_size);

#endif /* BSAL_RED_BLACK_NODE_H */
