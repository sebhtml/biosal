
#include "red_black_tree_iterator.h"

#include "red_black_tree.h"
#include "red_black_node.h"

#include <core/system/memory.h>

#include <stdlib.h>

void bsal_red_black_tree_iterator_init(struct bsal_red_black_tree_iterator *self,
                struct bsal_red_black_tree *tree)
{
    struct bsal_red_black_node *node;

    bsal_stack_init(&self->stack, sizeof(struct bsal_red_black_node *));

    node = tree->root;
    self->tree = tree;

    while (!bsal_red_black_node_is_leaf(node)) {

        bsal_stack_push(&self->stack, &node);

        node = node->left_node;
    }
}

void bsal_red_black_tree_iterator_destroy(struct bsal_red_black_tree_iterator *self)
{
    bsal_stack_destroy(&self->stack);
}

int bsal_red_black_tree_iterator_has_next(struct bsal_red_black_tree_iterator *self)
{
    return !bsal_stack_empty(&self->stack);
}

int bsal_red_black_tree_iterator_next(struct bsal_red_black_tree_iterator *self, void **key, void **value)
{
    struct bsal_red_black_node *node;
    struct bsal_red_black_node *node2;

    if (!bsal_red_black_tree_iterator_has_next(self))
        return 0;

    bsal_stack_pop(&self->stack, &node);

    node2 = node->right_node;

    while (!bsal_red_black_node_is_leaf(node2)) {

        bsal_stack_push(&self->stack, &node2);

        node2 = node2->left_node;
    }

    if (key != NULL)
        *key = node->key;
    if (value != NULL)
        *value = node->value;

    return 1;
}

int bsal_red_black_tree_iterator_get_next_key_and_value(struct bsal_red_black_tree_iterator *self, void *key, void *value)
{
    void *actual_key;
    void *actual_value;

    if (!bsal_red_black_tree_iterator_has_next(self))
        return 0;

    actual_key = NULL;
    actual_value = NULL;

    bsal_red_black_tree_iterator_next(self, &actual_key, &actual_value);

    if (key != NULL)
        bsal_memory_copy(key, actual_key, self->tree->key_size);
    if (value != NULL)
        bsal_memory_copy(value, actual_value, self->tree->value_size);

    return 1;
}


