
#include "red_black_tree_iterator.h"

#include "red_black_tree.h"
#include "red_black_node.h"

#include <core/system/memory.h>

#include <stdlib.h>

void core_red_black_tree_iterator_init(struct core_red_black_tree_iterator *self,
                struct core_red_black_tree *tree)
{
    struct core_red_black_node *node;

    core_stack_init(&self->stack, sizeof(struct core_red_black_node *));

    node = tree->root;
    self->tree = tree;

    while (!core_red_black_node_is_leaf(node)) {

        core_stack_push(&self->stack, &node);

        node = node->left_node;
    }
}

void core_red_black_tree_iterator_destroy(struct core_red_black_tree_iterator *self)
{
    core_stack_destroy(&self->stack);
}

int core_red_black_tree_iterator_has_next(struct core_red_black_tree_iterator *self)
{
    return !core_stack_empty(&self->stack);
}

int core_red_black_tree_iterator_next(struct core_red_black_tree_iterator *self, void **key, void **value)
{
    struct core_red_black_node *node;
    struct core_red_black_node *node2;

    if (!core_red_black_tree_iterator_has_next(self))
        return 0;

    core_stack_pop(&self->stack, &node);

    node2 = node->right_node;

    while (!core_red_black_node_is_leaf(node2)) {

        core_stack_push(&self->stack, &node2);

        node2 = node2->left_node;
    }

    if (key != NULL)
        *key = node->key;
    if (value != NULL)
        *value = node->value;

    return 1;
}

int core_red_black_tree_iterator_get_next_key_and_value(struct core_red_black_tree_iterator *self, void *key, void *value)
{
    void *actual_key;
    void *actual_value;

    if (!core_red_black_tree_iterator_has_next(self))
        return 0;

    actual_key = NULL;
    actual_value = NULL;

    core_red_black_tree_iterator_next(self, &actual_key, &actual_value);

    if (key != NULL)
        core_memory_copy(key, actual_key, self->tree->key_size);
    if (value != NULL)
        core_memory_copy(value, actual_value, self->tree->value_size);

    return 1;
}


