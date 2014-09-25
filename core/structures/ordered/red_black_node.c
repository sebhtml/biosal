
#include "red_black_node.h"

#include <core/system/debugger.h>
#include <core/system/memory_pool.h>
#include <core/system/memory.h>

#include <stdlib.h>

void bsal_red_black_node_init(struct bsal_red_black_node *self, int key_size, void *key, int value_size,
                void *value, struct bsal_memory_pool *pool)
{
    self->parent = NULL;
    self->left_node = NULL;
    self->right_node = NULL;

    self->key = bsal_memory_pool_allocate(pool, key_size);
    bsal_memory_copy(self->key, key, key_size);

    self->value = bsal_memory_pool_allocate(pool, value_size);

    if (value != NULL)
        bsal_memory_copy(self->value, value, value_size);

    self->color = BSAL_COLOR_RED;
}

void bsal_red_black_node_destroy(struct bsal_red_black_node *self, struct bsal_memory_pool  *pool)
{
    self->parent = NULL;
    self->left_node = NULL;
    self->right_node = NULL;

    bsal_memory_pool_free(pool, self->key);
    self->key = NULL;

    bsal_memory_pool_free(pool, self->value);
    self->value = NULL;

    self->color = BSAL_COLOR_NONE;
}

void *bsal_red_black_node_key(struct bsal_red_black_node *self)
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

void bsal_red_black_node_run_assertions(struct bsal_red_black_node *self, int key_size)
{
    if (self == NULL) {
        return;
    }
    if (self->left_node != NULL) {
        if (self->left_node->parent != self) {
            printf("Problem with %d -> %d (left_node parent should be %d, but it is %d)\n",
                            bsal_red_black_node_get_key_as_int(self, key_size), bsal_red_black_node_get_key_as_int(self->left_node, key_size),
                            bsal_red_black_node_get_key_as_int(self, key_size), bsal_red_black_node_get_key_as_int(self->left_node->parent, key_size));
        }
#if 1
        BSAL_DEBUGGER_ASSERT(self->left_node->parent == self);
#endif
    }
    if (self->right_node != NULL) {
        BSAL_DEBUGGER_ASSERT(self->right_node->parent == self);
    }

#if 0
    if (self->parent == NULL) {
        BSAL_DEBUGGER_ASSERT(self->color == BSAL_COLOR_BLACK);
    }

    if (self->color == BSAL_COLOR_RED) {
        BSAL_DEBUGGER_ASSERT(self->left_node == NULL || self->left_node->color == BSAL_COLOR_BLACK);
        BSAL_DEBUGGER_ASSERT(self->right_node == NULL || self->right_node->color == BSAL_COLOR_BLACK);
    }
#endif
}

int bsal_red_black_node_get_key_as_int(struct bsal_red_black_node *self, int key_size)
{
    int key;

    key = 0;

    if (key_size > (int)sizeof(key))
        key_size = sizeof(key);

    bsal_memory_copy(&key, self->key, key_size);

    return key;
}
