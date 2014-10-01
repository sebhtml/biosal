
#include "red_black_tree_iterator.h"

#include "red_black_tree.h"
#include "red_black_node.h"

#include <core/system/memory.h>

#include <stdlib.h>

void biosal_red_black_tree_iterator_init(struct biosal_red_black_tree_iterator *self,
                struct biosal_red_black_tree *tree)
{
    struct biosal_red_black_node *node;

    biosal_stack_init(&self->stack, sizeof(struct biosal_red_black_node *));

    node = tree->root;
    self->tree = tree;

    while (!biosal_red_black_node_is_leaf(node)) {

        biosal_stack_push(&self->stack, &node);

        node = node->left_node;
    }
}

void biosal_red_black_tree_iterator_destroy(struct biosal_red_black_tree_iterator *self)
{
    biosal_stack_destroy(&self->stack);
}

int biosal_red_black_tree_iterator_has_next(struct biosal_red_black_tree_iterator *self)
{
    return !biosal_stack_empty(&self->stack);
}

int biosal_red_black_tree_iterator_next(struct biosal_red_black_tree_iterator *self, void **key, void **value)
{
    struct biosal_red_black_node *node;
    struct biosal_red_black_node *node2;

    if (!biosal_red_black_tree_iterator_has_next(self))
        return 0;

    biosal_stack_pop(&self->stack, &node);

    node2 = node->right_node;

    while (!biosal_red_black_node_is_leaf(node2)) {

        biosal_stack_push(&self->stack, &node2);

        node2 = node2->left_node;
    }

    if (key != NULL)
        *key = node->key;
    if (value != NULL)
        *value = node->value;

    return 1;
}

int biosal_red_black_tree_iterator_get_next_key_and_value(struct biosal_red_black_tree_iterator *self, void *key, void *value)
{
    void *actual_key;
    void *actual_value;

    if (!biosal_red_black_tree_iterator_has_next(self))
        return 0;

    actual_key = NULL;
    actual_value = NULL;

    biosal_red_black_tree_iterator_next(self, &actual_key, &actual_value);

    if (key != NULL)
        biosal_memory_copy(key, actual_key, self->tree->key_size);
    if (value != NULL)
        biosal_memory_copy(value, actual_value, self->tree->value_size);

    return 1;
}


