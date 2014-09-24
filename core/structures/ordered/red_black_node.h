
#ifndef BSAL_RED_BLACK_NODE_H
#define BSAL_RED_BLACK_NODE_H

#define BSAL_COLOR_NONE     0
#define BSAL_COLOR_RED      1
#define BSAL_COLOR_BLACK    2

struct bsal_red_black_node;

/*
 * A red-black node.
 */
struct bsal_red_black_node {
    struct bsal_red_black_node *parent;
    struct bsal_red_black_node *left_child;
    struct bsal_red_black_node *right_child;
    char color;

    int key;

#if 0
    void *key;
    void *value;
#endif
};

void bsal_red_black_node_init(struct bsal_red_black_node *self, int key);
void bsal_red_black_node_destroy(struct bsal_red_black_node *self);

int bsal_red_black_node_key(struct bsal_red_black_node *self);
char bsal_red_black_node_color(struct bsal_red_black_node *self);
void bsal_red_black_node_set_color(struct bsal_red_black_node *self, char color);

struct bsal_red_black_node *bsal_red_black_node_left_child(struct bsal_red_black_node *self);
void bsal_red_black_node_set_left_child(struct bsal_red_black_node *self, struct bsal_red_black_node *node);

struct bsal_red_black_node *bsal_red_black_node_right_child(struct bsal_red_black_node *self);
void bsal_red_black_node_set_right_child(struct bsal_red_black_node *self, struct bsal_red_black_node *node);

struct bsal_red_black_node *bsal_red_black_node_parent(struct bsal_red_black_node *self);
void bsal_red_black_node_set_parent(struct bsal_red_black_node *self, struct bsal_red_black_node *node);

#endif /* BSAL_RED_BLACK_NODE_H */
