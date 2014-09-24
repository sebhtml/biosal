
#include "red_black_node.h"

#include <core/system/debugger.h>

#include <stdlib.h>

void bsal_red_black_node_init(struct bsal_red_black_node *self, int key)
{
    self->parent = NULL;
    self->left_node = NULL;
    self->right_node = NULL;

    self->key = key;

    self->color = BSAL_COLOR_RED;
}

void bsal_red_black_node_destroy(struct bsal_red_black_node *self)
{
    self->parent = NULL;
    self->left_node = NULL;
    self->right_node = NULL;

    self->key = -1;

    self->color = BSAL_COLOR_NONE;
}

int bsal_red_black_node_key(struct bsal_red_black_node *self)
{
    return self->key;
}

char bsal_red_black_node_color(struct bsal_red_black_node *self)
{
    return self->color;
}

void bsal_red_black_node_set_color(struct bsal_red_black_node *self, char color)
{
    self->color = color;
}

struct bsal_red_black_node *bsal_red_black_node_left_node(struct bsal_red_black_node *self)
{
    return self->left_node;
}

void bsal_red_black_node_set_left_node(struct bsal_red_black_node *self, struct bsal_red_black_node *node)
{
    self->left_node = node;
}

struct bsal_red_black_node *bsal_red_black_node_right_node(struct bsal_red_black_node *self)
{
    return self->right_node;
}

void bsal_red_black_node_set_right_node(struct bsal_red_black_node *self, struct bsal_red_black_node *node)
{
    self->right_node = node;
}

struct bsal_red_black_node *bsal_red_black_node_parent(struct bsal_red_black_node *self)
{
    return self->parent;
}

void bsal_red_black_node_set_parent(struct bsal_red_black_node *self, struct bsal_red_black_node *node)
{
    self->parent = node;
}

struct bsal_red_black_node *bsal_red_black_node_uncle(struct bsal_red_black_node *self)
{
    struct bsal_red_black_node *grandparent;

    grandparent = bsal_red_black_node_grandparent(self);

    if (grandparent == NULL)
        return NULL;

    if (self->parent == grandparent->left_node) {
        return grandparent->right_node;
    } else {
        return grandparent->left_node;
    }
}

struct bsal_red_black_node *bsal_red_black_node_grandparent(struct bsal_red_black_node *self)
{
    if (self != NULL && self->parent != NULL) {
        return self->parent->parent;
    } else {
        return NULL;
    }
}
