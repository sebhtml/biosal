
#include "red_black_node.h"

#include "red_black_tree.h"

#include <core/system/debugger.h>
#include <core/system/memory.h>

#include <stdlib.h>

void core_red_black_node_init(struct core_red_black_node *self, void *key, void *value)
{
    self->parent = NULL;
    self->left_node = NULL;
    self->right_node = NULL;

    self->key = key;
    self->value = value;

    self->color = CORE_COLOR_RED;

    /*
     * leaf nodes are NIL
     */
    if (self->key == NULL)
        self->color = CORE_COLOR_BLACK;
}

void core_red_black_node_destroy(struct core_red_black_node *self)
{
    self->parent = NULL;
    self->left_node = NULL;
    self->right_node = NULL;

    self->key = NULL;
    self->value = NULL;

    self->color = CORE_COLOR_NONE;
}

void *core_red_black_node_key(struct core_red_black_node *self)
{
    return self->key;
}

char core_red_black_node_color(struct core_red_black_node *self)
{
    return self->color;
}

void core_red_black_node_set_color(struct core_red_black_node *self, char color)
{
    self->color = color;
}

struct core_red_black_node *core_red_black_node_left_node(struct core_red_black_node *self)
{
    return self->left_node;
}

void core_red_black_node_set_left_node(struct core_red_black_node *self, struct core_red_black_node *node)
{
    CORE_DEBUGGER_ASSERT(self != NULL);
    CORE_DEBUGGER_ASSERT(node != NULL);

    self->left_node = node;

    node->parent = self;
}

struct core_red_black_node *core_red_black_node_right_node(struct core_red_black_node *self)
{
    return self->right_node;
}

void core_red_black_node_set_right_node(struct core_red_black_node *self, struct core_red_black_node *node)
{
    CORE_DEBUGGER_ASSERT(self != NULL);
    CORE_DEBUGGER_ASSERT(node != NULL);

    self->right_node = node;
    node->parent = self;
}

struct core_red_black_node *core_red_black_node_parent(struct core_red_black_node *self)
{
    return self->parent;
}

void core_red_black_node_set_parent(struct core_red_black_node *self, struct core_red_black_node *node)
{
    self->parent = node;
}

struct core_red_black_node *core_red_black_node_uncle(struct core_red_black_node *self)
{
    struct core_red_black_node *grandparent;

    grandparent = core_red_black_node_grandparent(self);

    if (grandparent == NULL)
        return NULL;

    if (self->parent == grandparent->left_node) {
        return grandparent->right_node;
    } else {
        return grandparent->left_node;
    }
}

struct core_red_black_node *core_red_black_node_grandparent(struct core_red_black_node *self)
{
    if (self != NULL && self->parent != NULL) {
        return self->parent->parent;
    } else {
        return NULL;
    }
}

void core_red_black_node_run_assertions(struct core_red_black_node *self, struct core_red_black_tree *tree)
{
    int key_size;

    key_size = tree->key_size;

    if (self == NULL) {
        return;
    }

    if (core_red_black_node_is_leaf(self))
        return;

    if (!core_red_black_node_is_leaf(self->left_node)) {
        if (self->left_node->parent != self) {
            printf("Problem with %d -> %d (left_node parent should be %d, but it is %d)\n",
                            core_red_black_node_get_key_as_int(self, key_size), core_red_black_node_get_key_as_int(self->left_node, key_size),
                            core_red_black_node_get_key_as_int(self, key_size), core_red_black_node_get_key_as_int(self->left_node->parent, key_size));
        }
#if 1
        CORE_DEBUGGER_ASSERT(self->left_node->parent == self);
#endif

        CORE_DEBUGGER_ASSERT(core_red_black_tree_compare(tree, self->left_node->key, self->key) <= 0);
        /*CORE_DEBUGGER_ASSERT(core_red_black_tree_compare(tree, self->left_node->key, tree->root->key) <= 0);*/
    }
    if (!core_red_black_node_is_leaf(self->right_node)) {
        CORE_DEBUGGER_ASSERT(self->right_node->parent == self);
        /*CORE_DEBUGGER_ASSERT(core_red_black_tree_compare(tree, self->right_node->key, self->key) >= 0);*/
    }

#if 0
    if (self->parent == NULL) {
        CORE_DEBUGGER_ASSERT(self->color == CORE_COLOR_BLACK);
    }

    if (self->color == CORE_COLOR_RED) {
        CORE_DEBUGGER_ASSERT(self->left_node == NULL || self->left_node->color == CORE_COLOR_BLACK);
        CORE_DEBUGGER_ASSERT(self->right_node == NULL || self->right_node->color == CORE_COLOR_BLACK);
    }
#endif
}

int core_red_black_node_get_key_as_int(struct core_red_black_node *self, int key_size)
{
    int key;

    key = 0;

    if (key_size > (int)sizeof(key))
        key_size = sizeof(key);

    core_memory_copy(&key, self->key, key_size);

    return key;
}

struct core_red_black_node *core_red_black_node_sibling(struct core_red_black_node *self)
{
    /*
     * The root has no sibling.
     */
    if (self->parent == NULL)
        return NULL;

    /*
     * The current node is the left_node.
     */
    if (self == self->parent->left_node)
        return self->parent->right_node;

    /*
     * The current node is the right_node
     */
    if (self == self->parent->right_node)
        return self->parent->left_node;

    /*
     * This statement is not reachable.
     */
    return NULL;
}

int core_red_black_node_is_red(struct core_red_black_node *self)
{
    return self != NULL && self->color == CORE_COLOR_RED;
}

int core_red_black_node_is_black(struct core_red_black_node *self)
{
    return self == NULL || self->color == CORE_COLOR_BLACK;
}

int core_red_black_node_is_leaf(struct core_red_black_node *self)
{
    CORE_DEBUGGER_ASSERT(self != NULL);

    return self->key == NULL;
}

int core_red_black_node_is_left_node(struct core_red_black_node *self)
{
    if (self->parent != NULL
                    && self->parent->left_node == self)
        return 1;

    return 0;
}

int core_red_black_node_is_right_node(struct core_red_black_node *self)
{
    if (self->parent != NULL
                    && self->parent->right_node == self)
        return 1;

    return 0;
}

int core_red_black_node_is_root(struct core_red_black_node *self)
{
    return self->parent == NULL;
}

void core_red_black_node_print(struct core_red_black_node *self, int key_size)
{
    if (core_red_black_node_is_leaf(self))
        printf("NIL");
    else
        printf("%d", core_red_black_node_get_key_as_int(self, key_size));

    if (core_red_black_node_is_red(self))
        printf(" RED");
    else
        printf(" BLACK");

    printf(" self %p parent %p left_node %p right_node %p\n",
                    (void *)self, (void *)self->parent,
                    (void *)self->left_node, (void *)self->right_node);
}
